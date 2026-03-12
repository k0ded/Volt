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
		uint32_t padding[3];
	};

	void GenerateMipMaps(RefPtr<RHI::Image> image)
	{
		const RHI::ImageDesc& imageDesc = image->GetDesc();

		if (!VT_CHECK(imageDesc.mips > 1))
		{
			return;
		}

		VT_ENSURE_MSG(imageDesc.usage != RHI::ImageUsage::Texture, "ImageUsage::Texture does not support mip map generation!");

		GlobalMemoryStackMark memMark;

		RefPtr<PooledCommandBuffer> pooledCommandBuffer = CommandBufferPool::GetCommandBuffer();
		RefPtr<RHI::CommandBuffer> commandBuffer = pooledCommandBuffer->Get();

		commandBuffer->Begin();

		const uint32_t numMips = imageDesc.mips;

		// Transition mip 0
		{
			RHI::ResourceState initialImageState = image->GetResourceStateTracker().GetResourceState(0);

			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
			barrier.imageBarrier().srcAccess = initialImageState.access;
			barrier.imageBarrier().srcStage = initialImageState.stage;
			barrier.imageBarrier().srcLayout = initialImageState.layout;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderWrite;
			barrier.imageBarrier().dstStage = RHI::BarrierStage::ComputeShader;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::ShaderWrite;
			barrier.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrier });
		}

		RefPtr<RHI::ComputePipeline> pipeline = PipelineStateCache::GetComputePipeline(Renderer::GetDefaultResources().generateMipMapsShader);

		STRING_HASH_CONSTEXPR StringHash SourceMipStringHash = StringHash::Construct("SourceMip");
		STRING_HASH_CONSTEXPR StringHash DestinationMipStringHash = StringHash::Construct("RWDstMip");

		const RHI::ShaderResourceBinding* sourceMipBinding = pipeline->GetResourceBindingFromName(SourceMipStringHash);
		const RHI::ShaderResourceBinding* destinationMipBinding = pipeline->GetResourceBindingFromName(DestinationMipStringHash);

		//VT_ENSURE(sourceMipBinding && destinationMipBinding);

		// Create state per mip.
		RHI::UniformBufferDesc desc{};
		desc.size = sizeof(GenerateMipMapsGlobals) * (numMips - 1);
		desc.debugName = "GlobalsBuffer";

		RefPtr<RHI::UniformBuffer> uniformBuffer = RHI::UniformBuffer::Create(desc);

		GenerateMipMapsGlobals* globals = uniformBuffer->Map<GenerateMipMapsGlobals>();

		for (uint32_t i = 1; i < numMips; ++i)
		{
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
			shaderBindingMap.SetUniformBufferWithSizeAndOffset(RHI::ShaderStage::Compute, 0, uniformBuffer->GetView(), sizeof(GenerateMipMapsGlobals), sizeof(GenerateMipMapsGlobals) * (i - 1));
			
			auto srcView = image->GetView(srcViewDesc);
			auto dstView = image->GetView(dstViewDesc);

			shaderBindingMap.SetTextureUAV(RHI::ShaderStage::Compute, sourceMipBinding->binding, srcView);
			shaderBindingMap.SetTextureUAV(RHI::ShaderStage::Compute, destinationMipBinding->binding, dstView);

			commandBuffer->BindPipeline(pipeline);
			commandBuffer->BindShaderBindings(shaderBindingMap);
			commandBuffer->Dispatch(Math::DivideRoundUp(dstMipWidth, 8u), Math::DivideRoundUp(dstMipHeight, 8u), 1);
		
			if (i < numMips - 1)
			{
				RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsGlobalBarrier();
				barrier.globalBarrier().srcStage = RHI::BarrierStage::ComputeShader;
				barrier.globalBarrier().srcAccess = RHI::BarrierAccess::ShaderWrite;
				barrier.globalBarrier().dstStage = RHI::BarrierStage::ComputeShader;
				barrier.globalBarrier().dstAccess = RHI::BarrierAccess::ShaderWrite;

				commandBuffer->ResourceBarrier({ barrier });
			}
		}

		// Transition back to initial state
		{
			RHI::ResourceState initialImageState = image->GetResourceStateTracker().GetResourceState(0);

			RHI::ResourceBarrierInfo barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
			barrier.imageBarrier().srcAccess = RHI::BarrierAccess::ShaderWrite;
			barrier.imageBarrier().srcStage = RHI::BarrierStage::ComputeShader;
			barrier.imageBarrier().srcLayout = RHI::ImageLayout::ShaderWrite;
			barrier.imageBarrier().dstAccess = initialImageState.access;
			barrier.imageBarrier().dstStage = initialImageState.stage;
			barrier.imageBarrier().dstLayout = initialImageState.layout;
			barrier.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrier });
		}

		uniformBuffer->Unmap();
		commandBuffer->End();
		RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFenceAndWait(commandBuffer);
	}
}
