#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/LightCullingTechnique.h"

#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/RendererCommon.h"

#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	LightCullingTechnique::LightCullingTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph), m_blackboard(blackboard)
	{
	}

	LightCullingData LightCullingTechnique::Execute()
	{
		const auto& uniformBuffers = m_blackboard.Get<UniformBuffersData>();
		const auto& preDepthData = m_blackboard.Get<DepthPrePass>();
		const auto& viewUniformBuffer = m_blackboard.Get<ViewUniformBuffer>();
		const auto& gpuSceneData = m_blackboard.Get<GPUSceneData>();

		constexpr uint32_t MAX_LIGHT_COUNT_PER_TILE = 512;

		const uint32_t tileCountX = Math::DivideRoundUp(viewUniformBuffer.renderSize.x, TILE_SIZE);
		const uint32_t tileCountY = Math::DivideRoundUp(viewUniformBuffer.renderSize.y, TILE_SIZE);

		LightCullingData& data = m_renderGraph.AddPass<LightCullingData>("Light Culling",
		[&](RenderGraph::Builder& builder, LightCullingData& data) 
		{
			{
				const auto desc = RGUtils::CreateBufferDescGPU<uint32_t>(tileCountX * tileCountY * MAX_LIGHT_COUNT_PER_TILE, "Visible Point Lights");
				data.visibleLightsBuffer = builder.CreateBuffer(desc);
			}

			builder.ReadResource(preDepthData.depth);
			builder.ReadResource(uniformBuffers.viewDataBuffer);
			builder.ReadResource(gpuSceneData.lightsBuffer);

			builder.SetIsComputePass();
		},
		[=](const LightCullingData& data, RenderContext& context) 
		{
			auto pipeline = ShaderMap::GetComputePipeline("LightTileBinning");

			context.BindPipeline(pipeline);
			context.SetConstant("depthTexture"_sh, preDepthData.depth);
			context.SetConstant("viewData"_sh, uniformBuffers.viewDataBuffer);
			context.SetConstant("lightsBuffer"_sh, gpuSceneData.lightsBuffer);
			context.SetConstant("visibleLightIndices"_sh, data.visibleLightsBuffer);
			context.SetConstant("tileCount"_sh, glm::uvec2{ tileCountX, tileCountY });
		
			context.Dispatch(tileCountX, tileCountY, 1u);
		});

		return data;
	}
}
