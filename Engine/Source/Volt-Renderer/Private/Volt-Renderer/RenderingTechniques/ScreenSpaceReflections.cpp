#include "vrpch.h"

#include "Volt-Renderer/RenderingTechniques/ScreenSpaceReflections.h"
#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/RendererCommon.h"
#include "Volt-Renderer/Renderer.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	ScreenSpaceReflections::ScreenSpaceReflections(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph), m_blackboard(blackboard)
	{
	}

	void ScreenSpaceReflections::Execute(RenderGraphImageHandle sceneColor)
	{
		const auto& gbufferData = m_blackboard.Get<GBufferData>();
		const auto& preDepthData = m_blackboard.Get<DepthPrePass>();
		const auto& viewData = m_blackboard.Get<ViewUniformBuffer>();
		const auto& uniformBuffers = m_blackboard.Get<UniformBuffersData>();

		const uint32_t width = static_cast<uint32_t>(viewData.renderSize.x);
		const uint32_t height = static_cast<uint32_t>(viewData.renderSize.y);

		m_renderGraph.AddPass("SSR Pass",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(sceneColor);
			builder.ReadResource(gbufferData.normals);
			builder.ReadResource(gbufferData.material);
			builder.ReadResource(preDepthData.depth);
			builder.ReadResource(uniformBuffers.viewDataBuffer);

			builder.SetIsComputePass();
			builder.SetHasSideEffect();
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("SSR");
			
			context.BindPipeline(pipeline);

			context.SetConstant("sceneNormals"_sh, gbufferData.normals);
			context.SetConstant("sceneDepth"_sh, preDepthData.depth);
			context.SetConstant("sceneMaterial"_sh, gbufferData.material);
			context.SetConstant("pointSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle());
			context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
			context.SetConstant("rwOutput"_sh, sceneColor);

			context.Dispatch(Math::DivideRoundUp(width, 8u), Math::DivideRoundUp(height, 8u), 1u);
		});
	}
}
