#include "ApplicationFixture.h"

#include <RenderCore/RenderGraph/RenderGraph.h>

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Images/ImageUtility.h>

using namespace Volt;

namespace IntegrationTests
{
	class RenderGraphFixture : public ApplicationFixture
	{
	};

	class TestingRenderGraph : public RenderGraph
	{
	public:
		VT_INLINE const Vector<Handle<RenderGraphPass>>& GetPasses() const { return m_passes; }
	};

	void ExpectAllPassesToBeCulled(const Vector<Handle<RenderGraphPass>>& passes)
	{
		for (const Handle<RenderGraphPass>& pass : passes)
		{
			EXPECT_EQ(pass->isCulled, true);
		}
	}

	void ExpectAllPassesToBeActive(const Vector<Handle<RenderGraphPass>>& passes)
	{
		for (const Handle<RenderGraphPass>& pass : passes)
		{
			EXPECT_EQ(pass->isCulled, false);
		}
	}

	template<typename ParameterStruct>
	void AddComputePass(RenderGraph& renderGraph, RenderGraphPassFlags flags, const ParameterStruct* passParameters)
	{
		renderGraph.AddPass("Compute Pass",
			RenderGraphPassFlags::Compute | flags,
			passParameters,
			[](RenderContext& context)
		{});
	}

	template<typename ParameterStruct>
	void AddRasterPass(RenderGraph& renderGraph, RenderGraphPassFlags flags, const ParameterStruct* passParameters)
	{
		renderGraph.AddPass("Raster Pass",
			flags,
			passParameters,
			[](RenderContext& context)
		{});
	}

	BEGIN_SHADER_PARAMETER_STRUCT(WriteSingleBufferParameters)
		SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWBuffer)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(ReadSingleBufferParameters)
		SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, Buffer)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(WriteSingleTextureParameters)
		SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWTexture)
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(RenderTargetParameters)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()

	BEGIN_SHADER_PARAMETER_STRUCT(RenderTargetWithSingleTextureReadParameters)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, Texture)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()

	TEST_F(RenderGraphFixture, PassIsCulled)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		TestingRenderGraph renderGraph{ commandBuffer };

		RGBufferRef buffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1));

		WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
		passParameters->RWBuffer = renderGraph.CreateUAV(buffer, RHI::PixelFormat::R32_UINT);

		AddComputePass(renderGraph, RenderGraphPassFlags::None, passParameters);

		renderGraph.Compile();

		ExpectAllPassesToBeCulled(renderGraph.GetPasses());
	}

	TEST_F(RenderGraphFixture, PassIsNeverCulled)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		TestingRenderGraph renderGraph{ commandBuffer };

		RGBufferRef buffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1));

		WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
		passParameters->RWBuffer = renderGraph.CreateUAV(buffer, RHI::PixelFormat::R32_UINT);

		AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);

		renderGraph.Compile();

		ExpectAllPassesToBeActive(renderGraph.GetPasses());
	}

	TEST_F(RenderGraphFixture, ExtractedResourceWriteIsNeverCulled)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		TestingRenderGraph renderGraph{ commandBuffer };

		RGBufferRef buffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1));

		WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
		passParameters->RWBuffer = renderGraph.CreateUAV(buffer, RHI::PixelFormat::R32_UINT);

		AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);

		RefPtr<RHI::StorageBuffer> outBuffer;
		renderGraph.EnqueueBufferExtraction(buffer, &outBuffer);
		renderGraph.Compile();

		ExpectAllPassesToBeActive(renderGraph.GetPasses());
	}

	TEST_F(RenderGraphFixture, ReadAfterWriteIsCulled)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		TestingRenderGraph renderGraph{ commandBuffer };

		RGBufferRef writeBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1));

		// Write pass
		{
			WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
			passParameters->RWBuffer = renderGraph.CreateUAV(writeBuffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::None, passParameters);
		}

		// Read pass
		{
			ReadSingleBufferParameters* passParameters = renderGraph.AllocParameters<ReadSingleBufferParameters>();
			passParameters->Buffer = renderGraph.CreateSRV(writeBuffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::None, passParameters);
		}

		renderGraph.Compile();

		ExpectAllPassesToBeCulled(renderGraph.GetPasses());
	}

	// #TODO_Ivar: This will fail with the current culling logic,
	//			   haven't figured out a good way to make this work and also
	//			   have RasterPassWithExtractUsingPreviousPassesResultIsNeverCulled working.
#if 0
	TEST_F(RenderGraphFixture, WriteAfterWriteIsCulled)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		TestingRenderGraph renderGraph{ commandBuffer };

		RGBufferRef writeBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1));

		// Write pass
		{
			WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
			passParameters->RWBuffer = renderGraph.CreateUAV(writeBuffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::None, passParameters);
		}

		// Write pass
		{
			WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
			passParameters->RWBuffer = renderGraph.CreateUAV(writeBuffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::None, passParameters);
		}

		renderGraph.Compile();

		ExpectAllPassesToBeCulled(renderGraph.GetPasses());
	}
#endif

	TEST_F(RenderGraphFixture, WriteAfterWriteNeverCullIsNeverCulled)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		TestingRenderGraph renderGraph{ commandBuffer };

		RGBufferRef writeBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1));

		// Write pass
		{
			WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
			passParameters->RWBuffer = renderGraph.CreateUAV(writeBuffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::None, passParameters);
		}

		// Write pass
		{
			WriteSingleBufferParameters* passParameters = renderGraph.AllocParameters<WriteSingleBufferParameters>();
			passParameters->RWBuffer = renderGraph.CreateUAV(writeBuffer, RHI::PixelFormat::R32_UINT);

			AddComputePass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		ExpectAllPassesToBeActive(renderGraph.GetPasses());
	}

	TEST_F(RenderGraphFixture, MultipleProducerChainIsCulled)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		TestingRenderGraph renderGraph{ commandBuffer };

		RGTextureDesc textureDesc{};
		textureDesc.format = RHI::PixelFormat::R32G32B32A32_SFLOAT;
		textureDesc.width = 1024;
		textureDesc.height = 1024;
		textureDesc.mips = RHI::Utility::CalculateMipCount(textureDesc.width, textureDesc.height);

		RGTextureRef texture = renderGraph.CreateTexture(textureDesc);

		for (uint32_t i = 0; i < textureDesc.mips; ++i)
		{
			WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();

			RGTextureUAVDesc desc{};
			desc.textureResource = texture;
			desc.baseMipLevel = i;
			desc.mipCount = 1;
			passParameters->RWTexture = renderGraph.CreateUAV(desc);

			AddComputePass(renderGraph, RenderGraphPassFlags::None, passParameters);
		}

		renderGraph.Compile();

		ExpectAllPassesToBeCulled(renderGraph.GetPasses());
	}

	TEST_F(RenderGraphFixture, RasterPassWritesRasterOutputWithNeverCullFlagIsNeverCulled)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		TestingRenderGraph renderGraph{ commandBuffer };

		RGTextureRef colorTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(1024, 1024, RHI::ImageUsage::AttachmentStorage));
		RGTextureRef depthTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::D32_SFLOAT>(1024, 1024, RHI::ImageUsage::AttachmentStorage));

		// First raster pass
		{
			RenderTargetParameters* passParameters = renderGraph.AllocParameters<RenderTargetParameters>();
			passParameters->renderTargets.renderTargets[0] = colorTexture;
			passParameters->renderTargets.depthTarget = depthTexture;

			AddRasterPass(renderGraph, RenderGraphPassFlags::None, passParameters);
		}

		// Second raster pass
		{
			RGTextureRef colorTexture2 = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(1024, 1024, RHI::ImageUsage::AttachmentStorage));

			RenderTargetParameters* passParameters = renderGraph.AllocParameters<RenderTargetParameters>();
			passParameters->renderTargets.renderTargets[0] = colorTexture2;
			passParameters->renderTargets.depthTarget = depthTexture;

			AddRasterPass(renderGraph, RenderGraphPassFlags::NeverCull, passParameters);
		}

		renderGraph.Compile();

		ExpectAllPassesToBeActive(renderGraph.GetPasses());
	}

	TEST_F(RenderGraphFixture, RasterPassWithExtractUsingPreviousPassesResultIsNeverCulled)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		TestingRenderGraph renderGraph{ commandBuffer };

		RGTextureRef colorTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(1024, 1024, RHI::ImageUsage::AttachmentStorage));
		RGTextureRef depthTexture = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::D32_SFLOAT>(1024, 1024, RHI::ImageUsage::AttachmentStorage));

		// First raster pass
		{
			RenderTargetParameters* passParameters = renderGraph.AllocParameters<RenderTargetParameters>();
			passParameters->renderTargets.renderTargets[0] = colorTexture;
			passParameters->renderTargets.depthTarget = depthTexture;

			AddRasterPass(renderGraph, RenderGraphPassFlags::None, passParameters);
		}

		// Compute write
		{
			WriteSingleTextureParameters* passParameters = renderGraph.AllocParameters<WriteSingleTextureParameters>();
			passParameters->RWTexture = renderGraph.CreateUAV(colorTexture);

			AddComputePass(renderGraph, RenderGraphPassFlags::None, passParameters);
		}

		// Second raster pass
		RGTextureRef colorTexture2 = renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(1024, 1024, RHI::ImageUsage::AttachmentStorage));
		{

			RenderTargetWithSingleTextureReadParameters* passParameters = renderGraph.AllocParameters<RenderTargetWithSingleTextureReadParameters>();
			passParameters->Texture = renderGraph.CreateSRV(depthTexture);
			passParameters->renderTargets.renderTargets[0] = colorTexture2;

			AddRasterPass(renderGraph, RenderGraphPassFlags::None, passParameters);
		}

		RefPtr<RHI::Image> outImage;
		renderGraph.EnqueueTextureExtraction(colorTexture2, &outImage);
		renderGraph.Compile();

		ExpectAllPassesToBeActive(renderGraph.GetPasses());
	}

}
