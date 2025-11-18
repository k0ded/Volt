#include "vrpch.h"

#include "Volt-Renderer/Texture/TextureSerializerCommon.h"

#include <RenderCore/CommandBufferPool.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/Utility/ResourceUtility.h>

namespace Volt::TextureSerializerCommon
{
	struct TextureData
	{
		struct Mip
		{
			uint32_t width;
			uint32_t height;
			size_t dataSize;
			size_t dataOffset;
			const void* dataPtr = nullptr;
		};

		void SetupMips(const Vector<TextureMip>& inMips, const Buffer& buffer)
		{
			for (const auto& mip : inMips)
			{
				auto& newMip = mips.emplace_back();
				newMip.width = mip.width;
				newMip.height = mip.height;
				newMip.dataSize = mip.dataSize;
				newMip.dataOffset = mip.dataOffset;
				newMip.dataPtr = buffer.As<const void>(newMip.dataOffset);
			}
		}

		Vector<Mip> mips;
	};

	Buffer GetImageDataBuffer(RefPtr<RHI::Image> image, Vector<TextureMip>& outMips)
	{
		const uint32_t formatTexelBlockSize = RHI::Utility::GetFormatTexelBlockSize(image->GetFormat());
		const uint32_t formatTexelsPerBlock = RHI::Utility::GetFormatTexelsPerBlock(image->GetFormat());

		// Create per mip staging buffer
		Vector<Handle<RHI::Allocation>> stagingBuffers;
		stagingBuffers.resize(image->GetMipCount());

		size_t totalImageSize = 0;

		for (uint32_t i = 0; i < image->GetMipCount(); ++i)
		{
			const uint32_t width = std::max(image->GetWidth() >> i, 1u);
			const uint32_t height = std::max(image->GetHeight() >> i, 1u);

			const size_t mipSize = std::max(uint32_t(width * height * (float(formatTexelBlockSize) / float(formatTexelsPerBlock))), formatTexelBlockSize) * image->GetLayerCount();

			RHI::BufferDesc stagingDesc{};
			stagingDesc.count = 1;
			stagingDesc.elementSize = mipSize;
			stagingDesc.usage = RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::TransferDst;
			stagingDesc.memoryUsage = RHI::MemoryUsage::GPUToCPU;
			stagingDesc.debugName = "Staging Buffer";

			stagingBuffers[i] = RHI::GraphicsContext::GetDefaultAllocator()->CreateBuffer(stagingDesc);
			totalImageSize += mipSize;
		}

		RefPtr<PooledCommandBuffer> pooledCommandBuffer = CommandBufferPool::GetCommandBuffer();
		RefPtr<RHI::CommandBuffer> commandBuffer = pooledCommandBuffer->Get();

		commandBuffer->Begin();

		const auto& currentResourceState = RHI::GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(image);

		{
			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
			barrier.imageBarrier().srcStage = currentResourceState.stage;
			barrier.imageBarrier().srcAccess = currentResourceState.access;
			barrier.imageBarrier().srcLayout = currentResourceState.layout;
			barrier.imageBarrier().dstStage = RHI::BarrierStage::Copy;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::CopySource;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::CopySource;
			barrier.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrier });
		}

		size_t offset = 0;
		for (uint32_t i = 0; i < image->GetMipCount(); i++)
		{
			auto& newMip = outMips.emplace_back();
			newMip.width = std::max(image->GetWidth() >> i, 1u);
			newMip.height = std::max(image->GetHeight() >> i, 1u);
			newMip.dataOffset = offset;

			const size_t mipSize = std::max(uint32_t(newMip.width * newMip.height * (float(formatTexelBlockSize) / float(formatTexelsPerBlock))), formatTexelBlockSize) * image->GetLayerCount();
			newMip.dataSize = mipSize;
			offset += mipSize;

			commandBuffer->CopyImageToBuffer(image, stagingBuffers[i], 0, newMip.width, newMip.height, 1, i);
		}

		{
			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
			barrier.imageBarrier().srcAccess = RHI::BarrierAccess::CopySource;
			barrier.imageBarrier().srcLayout = RHI::ImageLayout::CopySource;
			barrier.imageBarrier().srcStage = RHI::BarrierStage::Copy;
			barrier.imageBarrier().dstStage = currentResourceState.stage;
			barrier.imageBarrier().dstAccess = currentResourceState.access;
			barrier.imageBarrier().dstLayout = currentResourceState.layout;
			barrier.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->End();
		RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFenceAndWait(commandBuffer);

		Buffer dataBuffer;

		// Copy per mip data to data buffer.
		dataBuffer.Resize(totalImageSize);
		for (uint32_t i = 0; i < image->GetMipCount(); ++i)
		{
			const auto& mip = outMips[i];

			void* data = stagingBuffers[i]->Map<void>();
			dataBuffer.Copy(data, mip.dataSize, mip.dataOffset);
			stagingBuffers[i]->Unmap();
		}

		for (const auto& stagingBuffer : stagingBuffers)
		{
			RHI::GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingBuffer);
		}

		return dataBuffer;
	}

	void UploadImageData(RefPtr<RHI::Image> image, RHI::PixelFormat format, const Vector<struct TextureMip>& mips, const Buffer& dataBuffer)
	{
		TextureData texData{};
		texData.SetupMips(mips, dataBuffer);

		const uint32_t formatTexelBlockSize = RHI::Utility::GetFormatTexelBlockSize(format);
		const uint32_t formatTexelsPerBlock = RHI::Utility::GetFormatTexelsPerBlock(format);

		RHI::ImageCopyData copyData{};
		for (uint32_t mipIndex = 0; const auto& mipData : texData.mips)
		{
			auto& subData = copyData.copySubData.emplace_back();
			subData.data = mipData.dataPtr;
			subData.rowPitch = mipData.width * uint32_t(float(formatTexelBlockSize) / float(formatTexelsPerBlock));
			subData.slicePitch = static_cast<uint32_t>(mipData.dataSize);
			subData.width = mipData.width;
			subData.height = mipData.height;
			subData.depth = 1;
			subData.subResource.baseArrayLayer = 0;
			subData.subResource.baseMipLevel = mipIndex;
			subData.subResource.layerCount = image->GetLayerCount();
			subData.subResource.levelCount = 1;

			mipIndex++;
		}

		RHI::BufferDesc stagingDesc{};
		stagingDesc.count = 1;
		stagingDesc.elementSize = RHI::GraphicsContext::GetDevice()->GetMaxRequiredStagingBufferSizeForImage(image);
		stagingDesc.usage = RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::TransferSrc;
		stagingDesc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
		stagingDesc.debugName = "Staging Alloc";

		Handle<RHI::Allocation> stagingAlloc = RHI::GraphicsContext::GetDefaultAllocator()->CreateBuffer(stagingDesc);

		RefPtr<PooledCommandBuffer> pooledCommandBuffer = CommandBufferPool::GetCommandBuffer();
		RefPtr<RHI::CommandBuffer> commandBuffer = pooledCommandBuffer->Get();

		commandBuffer->Begin();

		{
			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
			RHI::ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), image);

			barrier.imageBarrier().dstStage = RHI::BarrierStage::Copy;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::CopyDest;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::CopyDest;
			barrier.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->UploadTextureData(image, stagingAlloc, copyData);

		{
			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
			RHI::ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), image);

			barrier.imageBarrier().dstStage = RHI::BarrierStage::PixelShader;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
			barrier.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->End();
		RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);

		RHI::GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAlloc);
	}
}
