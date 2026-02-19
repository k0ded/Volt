#include "vrpch.h"

#include "Volt-Renderer/Texture/TextureSerializerCommon.h"

#include <RenderCore/CommandBufferPool.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/Buffers/Buffer.h>
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

		void SetupMips(const Vector<TextureMip>& inMips, const DataBuffer& buffer)
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

	DataBuffer GetImageDataBuffer(RefPtr<RHI::Image> image, Vector<TextureMip>& outMips)
	{
		const RHI::ImageDesc& imageDesc = image->GetDesc();

		const uint32_t formatTexelBlockSize = RHI::Utility::GetFormatTexelBlockSize(imageDesc.format);
		const uint32_t formatTexelsPerBlock = RHI::Utility::GetFormatTexelsPerBlock(imageDesc.format);

		// Create per mip staging buffer
		Vector<RefPtr<RHI::Buffer>> stagingBuffers;
		stagingBuffers.resize(imageDesc.mips);

		size_t totalImageSize = 0;

		for (uint32_t i = 0; i < imageDesc.mips; ++i)
		{
			const uint32_t width = std::max(image->GetWidth() >> i, 1u);
			const uint32_t height = std::max(image->GetHeight() >> i, 1u);

			const size_t mipSize = std::max(uint32_t(width * height * (float(formatTexelBlockSize) / float(formatTexelsPerBlock))), formatTexelBlockSize) * imageDesc.layers;

			RHI::BufferDesc stagingDesc{};
			stagingDesc.numElements = mipSize;
			stagingDesc.elementSize = 1;
			stagingDesc.usage = RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::TransferDst;
			stagingDesc.memoryUsage = RHI::MemoryUsage::GPUToCPU;
			stagingDesc.debugName = "Staging Buffer";

			stagingBuffers[i] = RHI::Buffer::Create(stagingDesc);
			totalImageSize += mipSize;
		}

		RefPtr<PooledCommandBuffer> pooledCommandBuffer = CommandBufferPool::GetCommandBuffer();
		RefPtr<RHI::CommandBuffer> commandBuffer = pooledCommandBuffer->Get();

		commandBuffer->Begin();

		const auto& currentResourceState = RHI::GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(image, 0);

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
		for (uint32_t i = 0; i < imageDesc.mips; i++)
		{
			auto& newMip = outMips.emplace_back();
			newMip.width = std::max(imageDesc.width >> i, 1u);
			newMip.height = std::max(imageDesc.height >> i, 1u);
			newMip.dataOffset = offset;

			const size_t mipSize = std::max(uint32_t(newMip.width * newMip.height * (float(formatTexelBlockSize) / float(formatTexelsPerBlock))), formatTexelBlockSize) * imageDesc.layers;
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

		DataBuffer dataBuffer;

		// Copy per mip data to data buffer.
		dataBuffer.Resize(totalImageSize);
		for (uint32_t i = 0; i < imageDesc.mips; ++i)
		{
			const auto& mip = outMips[i];

			void* data = stagingBuffers[i]->Map<void>();
			dataBuffer.Copy(data, mip.dataSize, mip.dataOffset);
			stagingBuffers[i]->Unmap();
		}

		return dataBuffer;
	}

	void UploadImageData(RefPtr<RHI::Image> image, RHI::PixelFormat format, const Vector<struct TextureMip>& mips, const DataBuffer& dataBuffer, bool waitForGPU)
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
			subData.subResource.layerCount = image->GetDesc().layers;
			subData.subResource.levelCount = 1;

			mipIndex++;
		}

		RHI::BufferDesc stagingDesc{};
		stagingDesc.numElements = 1;
		stagingDesc.elementSize = RHI::GraphicsContext::GetDevice()->GetMaxRequiredStagingBufferSizeForImage(image);
		stagingDesc.usage = RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::TransferSrc;
		stagingDesc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
		stagingDesc.debugName = "Staging Alloc";

		RefPtr<RHI::Buffer> stagingBuffer = RHI::Buffer::Create(stagingDesc);

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

		commandBuffer->UploadTextureData(image, stagingBuffer, copyData);

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

		if (waitForGPU)
		{
			RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFenceAndWait(commandBuffer);
		}
		else
		{
			RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
		}
	}
}
