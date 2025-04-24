#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/TAATechnique.h"

#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/RendererCommon.h"

#include "Volt-Renderer/Texture/Texture2D.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/RenderContextUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <RHIModule/Pipelines/RenderPipeline.h>

namespace Volt
{
	struct TAAResolveVSPS
	{
		BEGIN_SHADER_DEFINITION(TAAResolveVSPS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Utility/FullscreenTriangle_vs.hlsl", "main", RHI::ShaderStage::Vertex)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/PostProcessing/TAA/TAAResolve_ps.hlsl", "main", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, CurrentColor)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, PreviousColor)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, SceneDepth)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float2>, VelocityTexture)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, LinearSampler)
			SHADER_PARAMETER(uint2, RenderSize)
			SHADER_PARAMETER(uint, FrameIndex)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(TAAResolveVSPS)

	TAATechnique::TAATechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph), m_blackboard(blackboard)
	{
	}

	TAAData TAATechnique::Execute(RefPtr<RHI::Image> previousColor, RenderGraphImageHandle velocityTexture)
	{
		const auto& shadingData = m_blackboard.Get<ShadingOutputData>();
		const auto& depthPrePass = m_blackboard.Get<DepthPrePass>();
		const auto& viewUniformBuffer = m_blackboard.Get<ViewUniformBuffer>();

		TAAData& data = m_renderGraph.AddPass<TAAData>("TAA Pass",
		[&](RenderGraph::Builder& builder, TAAData& data) 
		{
			{
				const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::B10G11R11_UFLOAT_PACK32>(viewUniformBuffer.renderSize.x, viewUniformBuffer.renderSize.y, RHI::ImageUsage::AttachmentStorage, "TAA Output");
				data.taaOutput = builder.CreateImage(desc);
			}

			{
				const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::B10G11R11_UFLOAT_PACK32>(viewUniformBuffer.renderSize.x, viewUniformBuffer.renderSize.y, RHI::ImageUsage::AttachmentStorage, "TAA Accumulation");
				data.accumulationOutput = builder.CreateImage(desc);
			}

			if (!previousColor)
			{
				data.previousColor = builder.AddExternalImage(Renderer::GetDefaultResources().whiteTexture->GetImage());
			}
			else
			{
				data.previousColor = builder.AddExternalImage(previousColor);
			}

			builder.ReadResource(velocityTexture);
			builder.ReadResource(data.previousColor);
			builder.ReadResource(shadingData.colorOutput);
			builder.ReadResource(depthPrePass.depth);

		},
		[=](const TAAData& data, RenderContext& context) 
		{
			RenderingInfo info = context.CreateRenderingInfo(viewUniformBuffer.renderSize.x, viewUniformBuffer.renderSize.y, { data.taaOutput, data.accumulationOutput });

			RHI::RenderPipelineCreateInfo pipelineInfo;
			pipelineInfo.shader = ShaderMap::Get<TAAResolveVSPS>();
			pipelineInfo.depthMode = RHI::DepthMode::None;
			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			TAAResolveVSPS::Parameters parameters;
			parameters.CurrentColor = shadingData.colorOutput;
			parameters.PreviousColor = data.previousColor;
			parameters.SceneDepth = depthPrePass.depth;
			parameters.VelocityTexture = velocityTexture;
			parameters.LinearSampler = Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>()->GetResourceHandle();
			parameters.RenderSize = viewUniformBuffer.renderSize;
			parameters.FrameIndex = viewUniformBuffer.frameIndex;

			context.BeginRendering(info);

			RCUtils::DrawFullscreenTriangle(context, pipeline, [&](RenderContext& context) 
			{
				context.SetParameters<TAAResolveVSPS>(parameters);
			});

			context.EndRendering();
		});

		return data;
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
