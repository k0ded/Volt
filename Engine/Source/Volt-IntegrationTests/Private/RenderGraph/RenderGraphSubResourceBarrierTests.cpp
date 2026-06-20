#include "ApplicationFixture.h"
#include "RenderGraph/RenderGraphCommon.h"

#include <RHIModule/Images/ImageUtility.h>

#include <vector>

using namespace Volt;

namespace IntegrationTests
{
	class RenderGraphSubResourceBarrierFixture : public ApplicationFixture
	{
	};

	BEGIN_SHADER_PARAMETER_STRUCT(TwoTextureUAVParameters)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWTextureA)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWTextureB)
	END_SHADER_PARAMETER_STRUCT()

	// Creates a GPU-only (transient) storage texture with the requested mip/layer
	// count. Re-uses Create2D for the remaining defaults (depth = 1, etc.).
	static RGTextureDesc CreateStorageTexture(uint32_t mips, uint32_t layers)
	{
		RGTextureDesc desc = RGTextureDesc::Create2D<RHI::PixelFormat::R32G32B32A32_SFLOAT>(64, 64, RHI::ImageUsage::Storage, "SubResMergeTex");
		desc.mips = mips;
		desc.layers = layers;
		return desc;
	}

	static RGTextureUAVRef CreateTextureUAV(TestingRenderGraph& renderGraph, RGTextureRef texture, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount)
	{
		RGTextureUAVDesc desc{};
		desc.textureResource = texture;
		desc.baseMipLevel = baseMip;
		desc.mipCount = mipCount;
		desc.baseArrayLayer = baseLayer;
		desc.layerCount = layerCount;
		return renderGraph.CreateUAV(desc);
	}

	// Collects every image barrier targeting 'resource' in the pre-pass barriers of a compiled pass,
	// preserving the order in which they were emitted (which is the sub-resource iteration order).
	static std::vector<RHI::ImageBarrier> CollectImageBarriers(const RGCompiledPass& compiledPass, RGResourceRef resource)
	{
		std::vector<RHI::ImageBarrier> result;
		for (const auto& barrierInfo : compiledPass.prePassBarriers.GetBarriers())
		{
			if (barrierInfo.barrier.type == RHI::BarrierType::Image && barrierInfo.resource == resource)
			{
				result.emplace_back(barrierInfo.barrier.imageBarrier());
			}
		}

		return result;
	}

	static size_t CountBarriers(const RGCompiledPass& compiledPass)
	{
		return compiledPass.prePassBarriers.GetBarrierCount();
	}

	static void ExpectComputeWriteBarrier(const RHI::ImageBarrier& barrier, uint32_t baseMip, uint32_t levelCount, uint32_t baseLayer, uint32_t layerCount)
	{
		// First use of a transient texture: Undefined -> ShaderWrite via a compute pass.
		EXPECT_EQ(barrier.srcStage, RHI::BarrierStage::None);
		EXPECT_EQ(barrier.srcAccess, RHI::BarrierAccess::None);
		EXPECT_EQ(barrier.srcLayout, RHI::ImageLayout::Undefined);
		EXPECT_EQ(barrier.dstStage, RHI::BarrierStage::ComputeShader);
		EXPECT_EQ(barrier.dstAccess, RHI::BarrierAccess::ShaderWrite);
		EXPECT_EQ(barrier.dstLayout, RHI::ImageLayout::ShaderWrite);

		EXPECT_EQ(barrier.subResource.baseMipLevel, baseMip);
		EXPECT_EQ(barrier.subResource.levelCount, levelCount);
		EXPECT_EQ(barrier.subResource.baseArrayLayer, baseLayer);
		EXPECT_EQ(barrier.subResource.layerCount, layerCount);
	}

	// A UAV write covering every mip of a single-layer texture should collapse into ONE
	// image barrier spanning all mip levels (mips are contiguous in sub-resource index space).
	TEST_F(RenderGraphSubResourceBarrierFixture, MultiMipUAVWrite_MergesAllMipsIntoSingleBarrier)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef texture = renderGraph.CreateTexture(CreateStorageTexture(4, 1));

		WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
		passParameters->RWTexture = renderGraph.CreateUAV(texture);

		AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);

		renderGraph.Compile();

		const RGCompiledPass& pass = renderGraph.GetCompiledPasses()[0];
		const std::vector<RHI::ImageBarrier> barriers = CollectImageBarriers(pass, texture);

		ASSERT_EQ(CountBarriers(pass), 1u);
		ASSERT_EQ(barriers.size(), 1u);
		ExpectComputeWriteBarrier(barriers[0], 0u, 4u, 0u, 1u);
	}

	// A UAV write covering every layer of a single-mip texture should collapse into ONE
	// image barrier spanning all array layers.
	TEST_F(RenderGraphSubResourceBarrierFixture, MultiLayerUAVWrite_MergesAllLayersIntoSingleBarrier)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef texture = renderGraph.CreateTexture(CreateStorageTexture(1, 4));

		WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
		passParameters->RWTexture = renderGraph.CreateUAV(texture);

		AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);

		renderGraph.Compile();

		const RGCompiledPass& pass = renderGraph.GetCompiledPasses()[0];
		const std::vector<RHI::ImageBarrier> barriers = CollectImageBarriers(pass, texture);

		ASSERT_EQ(CountBarriers(pass), 1u);
		ASSERT_EQ(barriers.size(), 1u);
		ExpectComputeWriteBarrier(barriers[0], 0u, 1u, 0u, 4u);
	}

	// Writing only a sub-range of mips must produce a barrier that covers exactly that range,
	// not the whole texture.
	TEST_F(RenderGraphSubResourceBarrierFixture, PartialMipRangeUAVWrite_BarrierCoversOnlySelectedMips)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef texture = renderGraph.CreateTexture(CreateStorageTexture(4, 1));

		WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
		// Mips 1 and 2 only.
		passParameters->RWTexture = CreateTextureUAV(renderGraph, texture, 1u, 2u, 0u, 1u);

		AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);

		renderGraph.Compile();

		const RGCompiledPass& pass = renderGraph.GetCompiledPasses()[0];
		const std::vector<RHI::ImageBarrier> barriers = CollectImageBarriers(pass, texture);

		ASSERT_EQ(CountBarriers(pass), 1u);
		ASSERT_EQ(barriers.size(), 1u);
		ExpectComputeWriteBarrier(barriers[0], 1u, 2u, 0u, 1u);
	}

	// Two writes to non-adjacent mips (a gap in sub-resource index space) must NOT be merged;
	// they require two distinct image barriers.
	TEST_F(RenderGraphSubResourceBarrierFixture, NonAdjacentMips_ProduceSeparateBarriers)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef texture = renderGraph.CreateTexture(CreateStorageTexture(4, 1));

		TwoTextureUAVParameters* passParameters = renderGraph.AllocParameters<TwoTextureUAVParameters>();
		passParameters->RWTextureA = CreateTextureUAV(renderGraph, texture, 0u, 1u, 0u, 1u); // mip 0
		passParameters->RWTextureB = CreateTextureUAV(renderGraph, texture, 2u, 1u, 0u, 1u); // mip 2

		AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);

		renderGraph.Compile();

		const RGCompiledPass& pass = renderGraph.GetCompiledPasses()[0];
		const std::vector<RHI::ImageBarrier> barriers = CollectImageBarriers(pass, texture);

		ASSERT_EQ(CountBarriers(pass), 2u);
		ASSERT_EQ(barriers.size(), 2u);
		ExpectComputeWriteBarrier(barriers[0], 0u, 1u, 0u, 1u);
		ExpectComputeWriteBarrier(barriers[1], 2u, 1u, 0u, 1u);
	}

	// A write-after-write across every mip needs only execution synchronization (no layout change),
	// so it should collapse into a single global barrier rather than per-sub-resource image barriers.
	TEST_F(RenderGraphSubResourceBarrierFixture, MultiMipWriteAfterWrite_SingleGlobalBarrier)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef texture = renderGraph.CreateTexture(CreateStorageTexture(4, 1));

		{
			WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
			passParameters->RWTexture = renderGraph.CreateUAV(texture);
			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		{
			WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
			passParameters->RWTexture = renderGraph.CreateUAV(texture);
			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		const RGCompiledPass& secondPass = renderGraph.GetCompiledPasses()[1];

		ASSERT_EQ(CountBarriers(secondPass), 1u);

		const auto& barriers = secondPass.prePassBarriers.GetBarriers();
		ASSERT_EQ(barriers.size(), 1u);
		ASSERT_EQ(barriers[0].barrier.type, RHI::BarrierType::Global);

		const RHI::GlobalBarrier& globalBarrier = barriers[0].barrier.globalBarrier();
		EXPECT_EQ(globalBarrier.srcStage, RHI::BarrierStage::ComputeShader);
		EXPECT_EQ(globalBarrier.srcAccess, RHI::BarrierAccess::ShaderWrite);
		EXPECT_EQ(globalBarrier.dstStage, RHI::BarrierStage::ComputeShader);
		EXPECT_EQ(globalBarrier.dstAccess, RHI::BarrierAccess::ShaderWrite);
	}

	TEST_F(RenderGraphSubResourceBarrierFixture, NonRectangularSubResources_ShouldNotOverMerge)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef texture = renderGraph.CreateTexture(CreateStorageTexture(2, 2));

		TwoTextureUAVParameters* passParameters = renderGraph.AllocParameters<TwoTextureUAVParameters>();
		passParameters->RWTextureA = CreateTextureUAV(renderGraph, texture, 1u, 1u, 0u, 1u); // (mip1, layer0) -> index 1
		passParameters->RWTextureB = CreateTextureUAV(renderGraph, texture, 0u, 1u, 1u, 1u); // (mip0, layer1) -> index 2

		AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);

		renderGraph.Compile();

		const RGCompiledPass& pass = renderGraph.GetCompiledPasses()[0];
		const std::vector<RHI::ImageBarrier> barriers = CollectImageBarriers(pass, texture);

		// Correct behaviour: the two non-rectangular sub-resources are transitioned independently.
		ASSERT_EQ(barriers.size(), 2u);
		ExpectComputeWriteBarrier(barriers[0], 1u, 1u, 0u, 1u); // (mip1, layer0)
		ExpectComputeWriteBarrier(barriers[1], 0u, 1u, 1u, 1u); // (mip0, layer1)
	}
}
