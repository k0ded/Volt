#include "vrpch.h"

#include "Volt-Renderer/RenderingTechniques/ScreenSpaceReflections.h"
#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/RendererCommon.h"
#include "Volt-Renderer/Renderer.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	struct SSRCS
	{
		BEGIN_SHADER_DEFINITION(SSRCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Utility/FullscreenTriangle_vs.hlsl", "main", RHI::ShaderStage::Vertex)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/PostProcessing/SSR.hlsl", "main", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float3>, SceneNormals)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, SceneDepth)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float2>, SceneMaterial)
			SHADER_PARAMETER_SAMPLER(vt::TextureSampler, PointSampler)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<float3>, RWOutput)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(SSRCS)

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
			auto pipeline = ShaderMap::GetComputePipeline<SSRCS>();
			
			SSRCS::Parameters parameters;
			parameters.SceneNormals = gbufferData.normals;
			parameters.SceneDepth = preDepthData.depth;
			parameters.SceneMaterial = gbufferData.material;
			parameters.PointSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle();
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.RWOutput = sceneColor;

			context.BindPipeline(pipeline);
			context.SetParameters<SSRCS>(parameters);
			context.Dispatch(Math::DivideRoundUp(width, 8u), Math::DivideRoundUp(height, 8u), 1u);
		});
	}
}
