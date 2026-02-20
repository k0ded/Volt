#include "vrpch.h"

#include "Volt-Renderer/Utility/ImageUtility.h"
#include "Volt-Renderer/Renderer.h"

#include <RenderCore/CommandBufferPool.h>
#include <RenderCore/Shader/PipelineStateCache.h>

#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIHelpers.h>

namespace Volt::ImageUtility
{
	struct GenerateMipMapsGlobals
	{
		uint32_t srcWidth;
		uint32_t srcHeight;
		uint32_t dstWidth;
		uint32_t dstHeight;
		uint32_t srcMip;
	};

	void GenerateMipMaps(RefPtr<RHI::Image> image)
	{
		const RHI::ImageDesc& imageDesc = image->GetDesc();

		if (!VT_CHECK(imageDesc.mips > 1))
		{
			return;
		}

		RefPtr<PooledCommandBuffer> pooledCommandBuffer = CommandBufferPool::GetCommandBuffer();
		RefPtr<RHI::CommandBuffer> commandBuffer = pooledCommandBuffer->Get();

		commandBuffer->Begin();

		const uint32_t numMips = imageDesc.mips;

		// Transition mip 0
		{
			RHI::ResourceState initialImageState = RHI::GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(image, 0);

			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
			barrier.imageBarrier().srcAccess = initialImageState.access;
			barrier.imageBarrier().srcStage = initialImageState.stage;
			barrier.imageBarrier().srcLayout = initialImageState.layout;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
			barrier.imageBarrier().dstStage = RHI::BarrierStage::ComputeShader;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
			barrier.imageBarrier().resource = image;
			barrier.imageBarrier().subResource.baseMipLevel = 0;
			barrier.imageBarrier().subResource.levelCount = 1;

			commandBuffer->ResourceBarrier({ barrier });
		}

		RefPtr<RHI::ComputePipeline> pipeline = PipelineStateCache::GetComputePipeline(Renderer::GetDefaultResources().generateMipMapsShader);

		STRING_HASH_CONSTEXPR StringHash SourceMipStringHash = StringHash::Construct("SourceMip");
		STRING_HASH_CONSTEXPR StringHash DestinationMipStringHash = StringHash::Construct("RWDstMip");

		const RHI::ShaderResourceBinding* sourceMipBinding = pipeline->GetResourceBindingFromName(SourceMipStringHash);
		const RHI::ShaderResourceBinding* destinationMipBinding = pipeline->GetResourceBindingFromName(DestinationMipStringHash);

		VT_ENSURE(sourceMipBinding && destinationMipBinding);

		// Create state per mip.
		RHI::UniformBufferDesc desc{};
		desc.size = sizeof(uint32_t) * 5 * numMips - 1;
		desc.debugName = "GlobalsBuffer";

		RefPtr<RHI::UniformBuffer> uniformBuffer = RHI::UniformBuffer::Create(desc);

		GenerateMipMapsGlobals* globals = uniformBuffer->Map<GenerateMipMapsGlobals>();

		for (uint32_t i = 1; i < numMips; ++i)
		{
			const uint32_t dstSubResourceIndex = RHI::GetSubResourceIndex(i - 1, 0, 0, imageDesc.mips, imageDesc.layers);

			RHI::ResourceBarrierInfo dstBarrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();

			// Dst barrier
			{
				RHI::ResourceState initialImageState = RHI::GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(image, dstSubResourceIndex);

				dstBarrier.imageBarrier().srcAccess = initialImageState.access;
				dstBarrier.imageBarrier().srcStage = initialImageState.stage;
				dstBarrier.imageBarrier().srcLayout = initialImageState.layout;
				dstBarrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderWrite;
				dstBarrier.imageBarrier().dstStage = RHI::BarrierStage::ComputeShader;
				dstBarrier.imageBarrier().dstLayout = RHI::ImageLayout::ShaderWrite;
				dstBarrier.imageBarrier().resource = image;
				dstBarrier.imageBarrier().subResource.baseMipLevel = i;
				dstBarrier.imageBarrier().subResource.levelCount = 1;

				commandBuffer->ResourceBarrier({ dstBarrier });
			}

			const uint32_t srcMipWidth = imageDesc.width >> (i - 1);
			const uint32_t srcMipHeight = imageDesc.height >> (i - 1);
			const uint32_t dstMipWidth = imageDesc.width >> i;
			const uint32_t dstMipHeight = imageDesc.height >> i;

			globals->srcWidth = srcMipWidth;
			globals->srcHeight = srcMipHeight;
			globals->dstWidth = dstMipWidth;
			globals->dstHeight = dstMipHeight;
			globals->srcMip = i - 1;
			globals++;

			RHI::ImageViewDesc srcViewDesc{};
			srcViewDesc.baseMipLevel = i - 1;
			srcViewDesc.mipCount = 1;

			RHI::ImageViewDesc dstViewDesc{};
			dstViewDesc.baseMipLevel = i;
			dstViewDesc.mipCount = 1;

			RHI::ShaderBindingMap shaderBindingMap = RHI::ShaderBindingMap::InitializeFromPipeline(pipeline);
			shaderBindingMap.SetUniformBufferWithSizeAndOffset(RHI::ShaderStage::Compute, 0, uniformBuffer->GetView(), sizeof(GenerateMipMapsGlobals), sizeof(GenerateMipMapsGlobals) * i);
			shaderBindingMap.SetTextureSRV(RHI::ShaderStage::Compute, sourceMipBinding->binding, image->GetView(srcViewDesc));
			shaderBindingMap.SetTextureUAV(RHI::ShaderStage::Compute, destinationMipBinding->binding, image->GetView(dstViewDesc));

			commandBuffer->BindPipeline(pipeline);
			commandBuffer->BindShaderBindings(shaderBindingMap);
			commandBuffer->Dispatch(Math::DivideRoundUp(dstMipWidth, 8u), Math::DivideRoundUp(dstMipHeight, 8u), 1);
		
			// Swap barriers
			{
				std::swap(dstBarrier.imageBarrier().srcAccess, dstBarrier.imageBarrier().dstAccess);
				std::swap(dstBarrier.imageBarrier().srcStage, dstBarrier.imageBarrier().dstStage);
				std::swap(dstBarrier.imageBarrier().srcLayout, dstBarrier.imageBarrier().dstLayout);
				dstBarrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
				dstBarrier.imageBarrier().dstStage = RHI::BarrierStage::ComputeShader;
				dstBarrier.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;

				commandBuffer->ResourceBarrier({ dstBarrier });
			}
		}

		uniformBuffer->Unmap();
		commandBuffer->End();
		RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFenceAndWait(commandBuffer);
	}
}
