#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/LightCullingTechnique.h"

#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/RendererCommon.h"

#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	struct LightTileBinningCS
	{
		BEGIN_SHADER_DEFINITION(LightTileBinningCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Lights/LightTileBinning.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, DepthTexture)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
			SHADER_PARAMETER_BUFFER(vt::TypedBuffer<LightDrawData>, LightsBuffer)
			SHADER_PARAMETER_BUFFER(vt::RWTypedBuffer<int>, RWVisibleLightIndices)
			SHADER_PARAMETER(uint2, TileCount)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(LightTileBinningCS)

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
			auto pipeline = ShaderMap::GetComputePipeline<LightTileBinningCS>();

			LightTileBinningCS::Parameters parameters;
			parameters.DepthTexture = preDepthData.depth;
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.LightsBuffer = gpuSceneData.lightsBuffer;
			parameters.RWVisibleLightIndices = data.visibleLightsBuffer;
			parameters.TileCount = glm::uvec2{ tileCountX, tileCountY };

			context.BindPipeline(pipeline);
			context.SetParameters<LightTileBinningCS>(parameters);
			context.Dispatch(tileCountX, tileCountY, 1u);
		});

		return data;
	}
}
