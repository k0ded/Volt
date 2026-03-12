#include "ApplicationFixture.h"
#include "RenderGraph/RenderGraphCommon.h"

#include <RHIModule/Buffers/Buffer.h>
#include <RHIModule/Images/ImageUtility.h>

#include <CoreUtilities/EnumUtils.h>

using namespace Volt;

namespace IntegrationTests
{
	class RenderGraphBarrierFixture : public ApplicationFixture
	{
	};

	constexpr RGResourceRef BarrierTypeGlobal = nullptr;

	class ExpectedBarriersMap
	{
	public:
		ExpectedBarriersMap(uint32_t passCount)
		{
			for (uint32_t i = 0; i < passCount; ++i)
			{
				m_barriers[i] = {};
			}
		}

		void AddImageBarrier(uint32_t passIndex, RGResourceRef resource, RHI::BarrierStage srcStage, RHI::BarrierAccess srcAccess, RHI::ImageLayout srcLayout, RHI::BarrierStage dstStage, RHI::BarrierAccess dstAccess, RHI::ImageLayout dstLayout)
		{
			m_barriers[passIndex][resource] = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
			m_barriers[passIndex][resource].imageBarrier().srcStage = srcStage;
			m_barriers[passIndex][resource].imageBarrier().dstStage = dstStage;
			m_barriers[passIndex][resource].imageBarrier().srcAccess = srcAccess;
			m_barriers[passIndex][resource].imageBarrier().dstAccess = dstAccess;
			m_barriers[passIndex][resource].imageBarrier().srcLayout = srcLayout;
			m_barriers[passIndex][resource].imageBarrier().dstLayout = dstLayout;

		}

		void AddImageBarrierNoSrcState(uint32_t passIndex, RGResourceRef resource, RHI::BarrierStage dstStage, RHI::BarrierAccess dstAccess, RHI::ImageLayout dstLayout)
		{
			AddImageBarrier(passIndex, resource, RHI::BarrierStage::None, RHI::BarrierAccess::None, RHI::ImageLayout::Undefined, dstStage, dstAccess, dstLayout);
		}

		void AddBufferBarrier(uint32_t passIndex, RGResourceRef resource, RHI::BarrierStage srcStage, RHI::BarrierAccess srcAccess, RHI::BarrierStage dstStage, RHI::BarrierAccess dstAccess)
		{
			m_barriers[passIndex][resource] = RHI::ResourceBarrierInfo::InitializeAsBufferBarrier();
			m_barriers[passIndex][resource].bufferBarrier().srcStage = srcStage;
			m_barriers[passIndex][resource].bufferBarrier().dstStage = dstStage;
			m_barriers[passIndex][resource].bufferBarrier().srcAccess = srcAccess;
			m_barriers[passIndex][resource].bufferBarrier().dstAccess = dstAccess;
		}

		void AddBufferBarrierNoSrcState(uint32_t passIndex, RGResourceRef resource, RHI::BarrierStage dstStage, RHI::BarrierAccess dstAccess)
		{
			AddBufferBarrier(passIndex, resource, RHI::BarrierStage::None, RHI::BarrierAccess::None, dstStage, dstAccess);
		}

		void AddGlobalBarrier(uint32_t passIndex, RHI::BarrierStage srcStage, RHI::BarrierAccess srcAccess, RHI::BarrierStage dstStage, RHI::BarrierAccess dstAccess)
		{
			ASSERT_FALSE(m_barriers[passIndex].contains(BarrierTypeGlobal));

			m_barriers[passIndex][BarrierTypeGlobal] = RHI::ResourceBarrierInfo::InitializeAsGlobalBarrier();
			m_barriers[passIndex][BarrierTypeGlobal].globalBarrier().srcStage = srcStage;
			m_barriers[passIndex][BarrierTypeGlobal].globalBarrier().dstStage = dstStage;
			m_barriers[passIndex][BarrierTypeGlobal].globalBarrier().srcAccess = srcAccess;
			m_barriers[passIndex][BarrierTypeGlobal].globalBarrier().dstAccess = dstAccess;
		}

		void AddGlobalBarrierNoSrcState(uint32_t passIndex, RHI::BarrierStage dstStage, RHI::BarrierAccess dstAccess)
		{
			AddGlobalBarrier(passIndex, RHI::BarrierStage::None, RHI::BarrierAccess::None, dstStage, dstAccess);
		}

		bool ContainsPass(uint32_t passIndex) const { return m_barriers.contains(passIndex); }
		const Map<RGResourceRef, RHI::ResourceBarrierInfo>& GetPassBarriers(uint32_t passIndex) const { return m_barriers.at(passIndex); }

	private:
		Map<uint32_t, Map<RGResourceRef, RHI::ResourceBarrierInfo>> m_barriers;
	};

	void VerifyGraphBarriers(const RGVector<RGCompiledPass>& compiledPasses, const ExpectedBarriersMap& expected)
	{
		for (uint32_t index = 0; const auto& pass : compiledPasses)
		{
			ASSERT_TRUE(expected.ContainsPass(index));
			const auto& expectedBarriers = expected.GetPassBarriers(index);
			const auto& barriers = pass.prePassBarriers.GetBarriers();
			
			ASSERT_EQ(barriers.size(), expectedBarriers.size());

			for (const auto& barrier : pass.prePassBarriers.GetBarriers())
			{
				ASSERT_TRUE(expectedBarriers.contains(barrier.resource));

				const auto& expectedBarrier = expectedBarriers.at(barrier.resource);

				ASSERT_EQ(barrier.barrier.type, expectedBarrier.type);

				switch (barrier.barrier.type)
				{
					case RHI::BarrierType::Image:
					{
						const RHI::ImageBarrier& actualBarrier = barrier.barrier.imageBarrier();
						const RHI::ImageBarrier& expectedImageBarrier = expectedBarrier.imageBarrier();

						EXPECT_EQ(actualBarrier.srcAccess, expectedImageBarrier.srcAccess);
						EXPECT_EQ(actualBarrier.dstAccess, expectedImageBarrier.dstAccess);
						EXPECT_EQ(actualBarrier.srcStage, expectedImageBarrier.srcStage);
						EXPECT_EQ(actualBarrier.dstStage, expectedImageBarrier.dstStage);
						EXPECT_EQ(actualBarrier.srcLayout, expectedImageBarrier.srcLayout);
						EXPECT_EQ(actualBarrier.dstLayout, expectedImageBarrier.dstLayout);

						break;
					}

					case RHI::BarrierType::Buffer:
					{
						const RHI::BufferBarrier& actualBarrier = barrier.barrier.bufferBarrier();
						const RHI::BufferBarrier& expectedBufferBarrier = expectedBarrier.bufferBarrier();

						EXPECT_EQ(actualBarrier.srcAccess, expectedBufferBarrier.srcAccess);
						EXPECT_EQ(actualBarrier.dstAccess, expectedBufferBarrier.dstAccess);
						EXPECT_EQ(actualBarrier.srcStage, expectedBufferBarrier.srcStage);
						EXPECT_EQ(actualBarrier.dstStage, expectedBufferBarrier.dstStage);

						break;
					}

					case RHI::BarrierType::Global:
					{
						const RHI::GlobalBarrier& globalBarrier = barrier.barrier.globalBarrier();
						const RHI::GlobalBarrier& expectedGlobalBarrier = expectedBarrier.globalBarrier();

						EXPECT_EQ(globalBarrier.srcAccess, expectedGlobalBarrier.srcAccess);
						EXPECT_EQ(globalBarrier.dstAccess, expectedGlobalBarrier.dstAccess);
						EXPECT_EQ(globalBarrier.srcStage,  expectedGlobalBarrier.srcStage);
						EXPECT_EQ(globalBarrier.dstStage,  expectedGlobalBarrier.dstStage);

						break;
					}
				}
			}

			index++;
		}
	}

	TEST_F(RenderGraphBarrierFixture, ComputeSingleTextureWrite)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef texture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R32G32B32A32_SFLOAT>(1024, 1024, RHI::ImageUsage::Storage));

		WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
		passParameters->RWTexture = renderGraph.CreateUAV(texture);

		AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddImageBarrierNoSrcState(0, texture, RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite, RHI::ImageLayout::ShaderWrite);

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}

	TEST_F(RenderGraphBarrierFixture, ComputeTextureReadAfterWrite)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef texture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R32G32B32A32_SFLOAT>(1024, 1024, RHI::ImageUsage::Storage));

		// Write pass
		{
			WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
			passParameters->RWTexture = renderGraph.CreateUAV(texture);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		// Read pass
		{
			ReadSingleTextureParameters* passParameters = renderGraph.AllocParameters<ReadSingleTextureParameters>();
			passParameters->Texture = renderGraph.CreateSRV(texture);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddImageBarrierNoSrcState(0, texture, RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite, RHI::ImageLayout::ShaderWrite);

		// We expect a image barrier here because the layout has to change for it to be read optimal.
		expectedBarriers.AddImageBarrier(1, texture,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite, RHI::ImageLayout::ShaderWrite,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderRead, RHI::ImageLayout::ShaderRead);

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}

	TEST_F(RenderGraphBarrierFixture, ComputeTextureWriteAfterWrite)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef texture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R32G32B32A32_SFLOAT>(1024, 1024, RHI::ImageUsage::Storage));

		// Write pass
		{
			WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
			passParameters->RWTexture = renderGraph.CreateUAV(texture);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		// Write pass
		{
			WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
			passParameters->RWTexture = renderGraph.CreateUAV(texture);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddImageBarrierNoSrcState(0, texture, RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite, RHI::ImageLayout::ShaderWrite);

		// We expect a global barrier here because the texture is already in the correct layout,
		// but we still need to wait for the previous pass to finish.
		expectedBarriers.AddGlobalBarrier(1,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite);

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}

	TEST_F(RenderGraphBarrierFixture, ComputeTextureReadAfterRead)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef texture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R32G32B32A32_SFLOAT>(1024, 1024, RHI::ImageUsage::Storage));

		// Initial Write pass
		{
			WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
			passParameters->RWTexture = renderGraph.CreateUAV(texture);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		// Read pass
		{
			ReadSingleTextureParameters* passParameters = renderGraph.AllocParameters<ReadSingleTextureParameters>();
			passParameters->Texture = renderGraph.CreateSRV(texture);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		// Second Read pass
		{
			ReadSingleTextureParameters* passParameters = renderGraph.AllocParameters<ReadSingleTextureParameters>();
			passParameters->Texture = renderGraph.CreateSRV(texture);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddImageBarrierNoSrcState(0, texture, RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite, RHI::ImageLayout::ShaderWrite);

		// We expect a image barrier here because the layout has to change for it to be read optimal.
		expectedBarriers.AddImageBarrier(1, texture,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite, RHI::ImageLayout::ShaderWrite,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderRead, RHI::ImageLayout::ShaderRead);

		// No barrier should exist for pass 2, because we are just reading again.

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}

	TEST_F(RenderGraphBarrierFixture, ComputeSingleBufferWrite)
	{
		TestingRenderGraph renderGraph{};

		RGBufferRef buffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1));

		WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
		passParameters->RWBuffer = renderGraph.CreateUAV(buffer, RHI::PixelFormat::R32_UINT);

		AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddGlobalBarrierNoSrcState(0, RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite);

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}

	TEST_F(RenderGraphBarrierFixture, ComputeBufferReadAfterWrite)
	{
		TestingRenderGraph renderGraph{};

		RGBufferRef buffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1));

		// Write pass
		{
			WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
			passParameters->RWBuffer = renderGraph.CreateUAV(buffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		// Read pass
		{
			ReadSingleBufferParameters* passParameters = renderGraph.AllocParameters<ReadSingleBufferParameters>();
			passParameters->Buffer = renderGraph.CreateSRV(buffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddGlobalBarrierNoSrcState(0, RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite);

		expectedBarriers.AddGlobalBarrier(1,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderRead);

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}

	TEST_F(RenderGraphBarrierFixture, ComputeBufferWriteAfterWrite)
	{
		TestingRenderGraph renderGraph{};

		RGBufferRef buffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1));

		// Write pass
		{
			WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
			passParameters->RWBuffer = renderGraph.CreateUAV(buffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		// Write pass
		{
			WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
			passParameters->RWBuffer = renderGraph.CreateUAV(buffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddGlobalBarrierNoSrcState(0, RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite);

		expectedBarriers.AddGlobalBarrier(1,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite);

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}

	TEST_F(RenderGraphBarrierFixture, ComputeBufferReadAfterRead)
	{
		TestingRenderGraph renderGraph{};

		RGBufferRef buffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1));

		// Initial Write pass
		{
			WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
			passParameters->RWBuffer = renderGraph.CreateUAV(buffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		// Read pass
		{
			ReadSingleBufferParameters* passParameters = renderGraph.AllocParameters<ReadSingleBufferParameters>();
			passParameters->Buffer = renderGraph.CreateSRV(buffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		// Second Read pass
		{
			ReadSingleBufferParameters* passParameters = renderGraph.AllocParameters<ReadSingleBufferParameters>();
			passParameters->Buffer = renderGraph.CreateSRV(buffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddGlobalBarrierNoSrcState(0, RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite);

		expectedBarriers.AddGlobalBarrier(1,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderWrite,
			RHI::BarrierStage::ComputeShader, RHI::BarrierAccess::ShaderRead);

		// No barrier should be added to pass 2, as it's not required when reading in sequence.

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}

	TEST_F(RenderGraphBarrierFixture, RasterRenderTargetWrite)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef colorTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(1024, 1024, RHI::ImageUsage::AttachmentStorage));
		RGTextureRef depthTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::D32_SFLOAT>(1024, 1024, RHI::ImageUsage::Attachment));

		RenderTargetParameters* passParameters = renderGraph.AllocParameters<RenderTargetParameters>();
		passParameters->renderTargets.renderTargets[0] = colorTexture;
		passParameters->renderTargets.depthTarget = depthTexture;

		AddRasterPass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddImageBarrierNoSrcState(0, colorTexture, RHI::BarrierStage::RenderTarget, RHI::BarrierAccess::RenderTarget, RHI::ImageLayout::RenderTarget);
		expectedBarriers.AddImageBarrierNoSrcState(0, depthTexture, RHI::BarrierStage::DepthStencil, RHI::BarrierAccess::DepthStencilWrite | RHI::BarrierAccess::DepthStencilRead, RHI::ImageLayout::DepthStencilWrite);

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(RasterRenderTargetReadAfterWrite_Parameters)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, ColorTexture)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, DepthTexture)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()

	TEST_F(RenderGraphBarrierFixture, RasterRenderTargetReadAfterWrite)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef colorTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(1024, 1024, RHI::ImageUsage::AttachmentStorage));
		RGTextureRef depthTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::D32_SFLOAT>(1024, 1024, RHI::ImageUsage::Attachment));

		// Write pass
		{
			RenderTargetParameters* passParameters = renderGraph.AllocParameters<RenderTargetParameters>();
			passParameters->renderTargets.renderTargets[0] = colorTexture;
			passParameters->renderTargets.depthTarget = depthTexture;

			AddRasterPass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}
		
		// Read pass
		RGTextureRef colorTexture2 = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(1024, 1024, RHI::ImageUsage::AttachmentStorage));
		{

			RasterRenderTargetReadAfterWrite_Parameters* passParameters = renderGraph.AllocParameters<RasterRenderTargetReadAfterWrite_Parameters>();
			passParameters->ColorTexture = renderGraph.CreateSRV(colorTexture);
			passParameters->DepthTexture = renderGraph.CreateSRV(depthTexture);
			passParameters->renderTargets.renderTargets[0] = colorTexture2;

			AddRasterPass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddImageBarrierNoSrcState(0, colorTexture, RHI::BarrierStage::RenderTarget, RHI::BarrierAccess::RenderTarget, RHI::ImageLayout::RenderTarget);
		expectedBarriers.AddImageBarrierNoSrcState(0, depthTexture, RHI::BarrierStage::DepthStencil, RHI::BarrierAccess::DepthStencilWrite | RHI::BarrierAccess::DepthStencilRead, RHI::ImageLayout::DepthStencilWrite);

		expectedBarriers.AddImageBarrier(1, colorTexture,
			RHI::BarrierStage::RenderTarget, RHI::BarrierAccess::RenderTarget, RHI::ImageLayout::RenderTarget,
			RHI::BarrierStage::VertexShader | RHI::BarrierStage::PixelShader, RHI::BarrierAccess::ShaderRead, RHI::ImageLayout::ShaderRead);

		expectedBarriers.AddImageBarrier(1, depthTexture,
			RHI::BarrierStage::DepthStencil, RHI::BarrierAccess::DepthStencilWrite | RHI::BarrierAccess::DepthStencilRead, RHI::ImageLayout::DepthStencilWrite,
			RHI::BarrierStage::VertexShader | RHI::BarrierStage::PixelShader, RHI::BarrierAccess::ShaderRead, RHI::ImageLayout::ShaderRead);

		expectedBarriers.AddImageBarrierNoSrcState(1, colorTexture2, RHI::BarrierStage::RenderTarget, RHI::BarrierAccess::RenderTarget, RHI::ImageLayout::RenderTarget);

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}

	TEST_F(RenderGraphBarrierFixture, RasterRenderTargetWriteAfterWrite)
	{
		TestingRenderGraph renderGraph{};

		RGTextureRef colorTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(1024, 1024, RHI::ImageUsage::AttachmentStorage));
		RGTextureRef depthTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::D32_SFLOAT>(1024, 1024, RHI::ImageUsage::Attachment));

		// Write pass
		{
			RenderTargetParameters* passParameters = renderGraph.AllocParameters<RenderTargetParameters>();
			passParameters->renderTargets.renderTargets[0] = colorTexture;
			passParameters->renderTargets.depthTarget = depthTexture;

			AddRasterPass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		// Read pass
		{

			RenderTargetParameters* passParameters = renderGraph.AllocParameters<RenderTargetParameters>();
			passParameters->renderTargets.renderTargets[0] = colorTexture;
			passParameters->renderTargets.depthTarget = depthTexture;

			AddRasterPass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		ExpectedBarriersMap expectedBarriers(renderGraph.GetNumCompiledPasses());
		expectedBarriers.AddImageBarrierNoSrcState(0, colorTexture, RHI::BarrierStage::RenderTarget, RHI::BarrierAccess::RenderTarget, RHI::ImageLayout::RenderTarget);
		expectedBarriers.AddImageBarrierNoSrcState(0, depthTexture, RHI::BarrierStage::DepthStencil, RHI::BarrierAccess::DepthStencilWrite | RHI::BarrierAccess::DepthStencilRead, RHI::ImageLayout::DepthStencilWrite);

		// We expect a global barrier here because we just want to wait for the previous shader
		// to execute.

		expectedBarriers.AddGlobalBarrier(1,
			RHI::BarrierStage::RenderTarget | RHI::BarrierStage::DepthStencil, RHI::BarrierAccess::RenderTarget | RHI::BarrierAccess::DepthStencilWrite | RHI::BarrierAccess::DepthStencilRead,
			RHI::BarrierStage::RenderTarget | RHI::BarrierStage::DepthStencil, RHI::BarrierAccess::RenderTarget | RHI::BarrierAccess::DepthStencilWrite | RHI::BarrierAccess::DepthStencilRead);

		VerifyGraphBarriers(renderGraph.GetCompiledPasses(), expectedBarriers);
	}
}
