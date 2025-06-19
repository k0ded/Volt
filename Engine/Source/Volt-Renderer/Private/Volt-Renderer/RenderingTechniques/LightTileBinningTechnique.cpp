#include "vrpch.h"

#include "Volt-Renderer/RenderingTechniques/LightTileBinningTechnique.h"
#include "Volt-Renderer/SceneRendererRenderGraphData.h"
#include "Volt-Renderer/RenderView.h"
#include "Volt-Renderer/RenderScene.h"

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>

namespace Volt
{
	struct LightTileBinningCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(GlobalShader)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SceneDepth)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<int>, RWVisibleLightIndices)
			SHADER_PARAMETER(uint2, TileCount)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(LightTileBinningCS, "Engine/Shaders/Source/Lights/LightTileBinning.hlsl", "MainCS", Compute);

	LightTileBinningTechnique::LightTileBinningTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard)
		: m_renderGraph(renderGraph), m_blackboard(blackboard)
	{
	}

	void LightTileBinningTechnique::Execute(const RenderView& view)
	{
		constexpr uint32_t MAX_LIGHT_COUNT_PER_TILE = 512;

		const SceneTextures& sceneTextures = m_blackboard.Get<SceneTextures>();

		const uint32_t numTilesX = Math::DivideRoundUp(view.width, TILE_SIZE);
		const uint32_t numTilesY = Math::DivideRoundUp(view.height, TILE_SIZE);

		RGBufferRef visibleLightIndices = m_renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<int32_t>(numTilesX * numTilesY * MAX_LIGHT_COUNT_PER_TILE, "VisibleLightIndices"));

		LightTileBinningCS::Parameters* passParameters = m_renderGraph.AllocParameters<LightTileBinningCS::Parameters>();
		passParameters->View = view.viewUniformBuffer;
		passParameters->SceneDepth = m_renderGraph.CreateSRV(sceneTextures.sceneDepth);
		passParameters->RWVisibleLightIndices = m_renderGraph.CreateUAV(visibleLightIndices, RHI::PixelFormat::R32_SINT);
		passParameters->GPUScene = view.renderScene->GetGPUSceneParameters(m_renderGraph);
		passParameters->TileCount = { numTilesX, numTilesY };

		auto shader = ShaderMap::Get<LightTileBinningCS>();
		ComputeShaderUtils::AddPass<LightTileBinningCS>(m_renderGraph,
			"LightTileBinning",
			shader,
			passParameters,
			{ numTilesX, numTilesY, 1 });

		LightScene& lightScene = m_blackboard.Add<LightScene>();
		lightScene.visibleLightIndices = visibleLightIndices;
	}
}
