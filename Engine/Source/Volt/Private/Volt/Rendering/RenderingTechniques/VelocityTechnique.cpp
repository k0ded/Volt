#include "vtpch.h"
#include "Volt/Rendering/RenderingTechniques/VelocityTechnique.h"

#include "Volt/Rendering/SceneRendererStructs.h"
#include "Volt/Rendering/Camera/Camera.h"
#include "Volt/Rendering/Renderer.h"
#include "Volt/Rendering/RendererCommon.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/RenderContextUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <RHIModule/Pipelines/RenderPipeline.h>

namespace Volt
{
	VelocityTechnique::VelocityTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph), m_blackboard(blackboard)
	{
	}

	RenderGraphResourceHandle VelocityTechnique::Execute(Ref<Camera> camera)
	{
		return ExecuteReprojectVelocity(camera);
	}

	RenderGraphResourceHandle VelocityTechnique::ExecuteReprojectVelocity(Ref<Camera> camera)
	{
		struct Output
		{
			RenderGraphImageHandle velocityTexture;
		};

		const auto& viewUniformBuffer = m_blackboard.Get<ViewUniformBuffer>();
		const auto& previousData = m_blackboard.Get<PreviousFrameData>();
		const auto& preDepthData = m_blackboard.Get<DepthPrePass>();

		const glm::uvec2 renderSize = viewUniformBuffer.renderSize;

		Output& reprojectData = m_renderGraph.AddPass<Output>("Reproject Velocity",
		[&](RenderGraph::Builder& builder, Output& data)
		{
			const auto desc = RGUtils::CreateImage2DDesc<RHI::PixelFormat::R16G16_SFLOAT>(renderSize.x, renderSize.y, RHI::ImageUsage::AttachmentStorage, "Velocity");
			data.velocityTexture = builder.CreateImage(desc);
		
			builder.ReadResource(preDepthData.depth);

			builder.SetHasSideEffect();
		},
		[=](const Output& data, RenderContext& context)
		{
			RenderingInfo info = context.CreateRenderingInfo(renderSize.x, renderSize.y, { data.velocityTexture });

			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shader = ShaderMap::Get("ReprojectVelocity");
			pipelineInfo.depthMode = RHI::DepthMode::None;
			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(info);

			RCUtils::DrawFullscreenTriangle(context, pipeline, [&](RenderContext& context)
			{
				context.SetConstant("inverseViewProjection"_sh, glm::inverse(camera->GetNonJitteredProjection() * camera->GetView()));
				context.SetConstant("previousViewProjection"_sh, previousData.viewProjection);
				context.SetConstant("renderSize"_sh, glm::vec2(renderSize));
				context.SetConstant("invRenderSize"_sh, 1.f / glm::vec2(renderSize));
				context.SetConstant("jitterOffset"_sh, (camera->GetSubpixelOffset() - previousData.jitter) * 0.5f);
				context.SetConstant("depthTexture"_sh, preDepthData.depth);
				context.SetConstant("pointSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle());
			});

			context.EndRendering();
		});

		return reprojectData.velocityTexture;
	}
}
