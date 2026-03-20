#include "vrpch.h"

#include "Volt-Renderer/RenderingTechniques/TAATechnique.h"
#include "Volt-Renderer/RenderingTechniques/TAANoise.h"
#include "Volt-Renderer/SceneRendererRenderGraphData.h"
#include "Volt-Renderer/RenderView.h"

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/SamplerStateCache.h>

namespace Volt
{
	struct TAAResolvePS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(TAAResolvePS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, CurrentColor)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, PreviousColor)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SceneDepth)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float2>, SceneVelocity)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
			SHADER_PARAMETER(uint2, RenderSize)
			SHADER_PARAMETER(uint, FrameIndex)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(TAAResolvePS, "Engine/Shaders/Source/PostProcessing/TAA/TAAResolve.hlsl", "MainPS", Pixel);

	BEGIN_SHADER_PARAMETER_STRUCT(TAAResolveParameters)
		SHADER_PARAMETER_STRUCT_INCLUDE(FullscreenTriangleVS::Parameters, VS)
		SHADER_PARAMETER_STRUCT_INCLUDE(TAAResolvePS::Parameters, PS)
	END_SHADER_PARAMETER_STRUCT()

	TAATechnique::TAATechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph), m_blackboard(blackboard)
	{
	}

	TAATechnique::Output TAATechnique::Execute(const RenderView& view, IntRef<RHI::Image> prevAccumulation)
	{
		SceneTextures& sceneTextures = m_blackboard.Get<SceneTextures>();

		// If this is the first frame, we write the scene color as the
		// accumulation (to be extracted) and return.
		if (!prevAccumulation)
		{
			Output result;
			result.accumulation = sceneTextures.sceneColor;

			return result;
		}

		RGTextureRef prevAccumulationTexture = m_renderGraph.RegisterExternalTexture(prevAccumulation);
		RGTextureRef accumulationTexture = m_renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::B10G11R11_UFLOAT_PACK32>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "TAA.Accumulation"));
		RGTextureRef outputTexture = m_renderGraph.CreateTexture(RGTextureDesc::Create2D<RHI::PixelFormat::R16G16B16A16_SFLOAT>(view.width, view.height, RHI::ImageUsage::AttachmentStorage, "TAA.Output"));

		TAAResolveParameters* passParameters = m_renderGraph.AllocParameters<TAAResolveParameters>();
		passParameters->PS.CurrentColor = m_renderGraph.CreateSRV(sceneTextures.sceneColor);
		passParameters->PS.PreviousColor = m_renderGraph.CreateSRV(prevAccumulationTexture);
		passParameters->PS.SceneDepth = m_renderGraph.CreateSRV(sceneTextures.sceneDepth);
		passParameters->PS.SceneVelocity = m_renderGraph.CreateSRV(sceneTextures.sceneVelocity);
		passParameters->PS.LinearSampler = SamplerStateCache::GetBilinearSampler();
		passParameters->PS.RenderSize = { view.width, view.height };
		passParameters->PS.FrameIndex = view.frameIndex;
		passParameters->PS.renderTargets.renderTargets[0] = outputTexture;
		passParameters->PS.renderTargets.renderTargets[1] = accumulationTexture;

		auto vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
		auto pixelShader = ShaderMap::Get<TAAResolvePS>();

		m_renderGraph.AddPass("TAA Resolve",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, view, pixelShader, vertexShader](RenderContext& context)
		{
			GraphicsPipelineState pipelineState;
			pipelineState.shaders = { vertexShader, pixelShader };
			pipelineState.cullMode = RHI::CullMode::None;
			pipelineState.depthMode = RHI::DepthMode::None;
			pipelineState.renderTargets = passParameters->PS.renderTargets;

			RenderingInfo renderingInfo = context.CreateRenderingInfo(view.width, view.height, passParameters->PS.renderTargets);

			context.BeginRendering(renderingInfo);
			context.SetPipelineState(pipelineState);
			context.SetParameters<TAAResolvePS>(pixelShader, &passParameters->PS);
			context.Draw(3, 1, 0, 0);
			context.EndRendering();
		});

		sceneTextures.sceneColor = outputTexture;

		Output result{};
		result.accumulation = accumulationTexture;

		return result;
	}
	
	TAANoise::TAANoise()
	{
		if (!s_initialized)
		{
			auto halton = [](size_t index, size_t base)
			{
				float f = 1.0f;
				float r = 0.0f;

				while (index > 0)
				{
					f /= base;
					r += f * (index % base);
					index /= base;
				}

				return r;
			};

			for (size_t i = 0; i < 8; ++i)
			{
				s_haltonX[i] = halton(i + 1, 2) * 2.0f - 1.0f;
				s_haltonY[i] = halton(i + 1, 3) * 2.0f - 1.0f;
			}

			s_initialized = true;
		}
	}

	glm::vec2 TAANoise::Get(uint32_t frameIndex, const glm::uvec2& renderSize)
	{
		return { s_haltonX[frameIndex % 8] / static_cast<float>(renderSize.x), s_haltonY[frameIndex % 8] / static_cast<float>(renderSize.y) };
	}
}
