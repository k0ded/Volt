#include "vtpch.h"
#include "Volt/Rendering/RenderingTechniques/TAATechnique.h"

#include "Volt/Rendering/SceneRendererStructs.h"
#include "Volt/Rendering/Renderer.h"
#include "Volt/Rendering/RendererCommon.h"

#include "Volt/Rendering/Texture/Texture2D.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/RenderContextUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <RHIModule/Pipelines/RenderPipeline.h>

namespace Volt
{
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
			pipelineInfo.shader = ShaderMap::Get("TAAResolve");
			pipelineInfo.depthMode = RHI::DepthMode::None;
			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);

			RCUtils::DrawFullscreenTriangle(context, pipeline, [&](RenderContext& context) 
			{
				context.SetConstant("currentColor"_sh, shadingData.colorOutput);
				context.SetConstant("previousColor"_sh, data.previousColor);
				context.SetConstant("sceneDepth"_sh, depthPrePass.depth);
				context.SetConstant("velocityTexture"_sh, velocityTexture);
				context.SetConstant("linearSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Clamp>()->GetResourceHandle());
				context.SetConstant("renderSize"_sh, viewUniformBuffer.renderSize);
				context.SetConstant("frameIndex"_sh, viewUniformBuffer.frameIndex);

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
