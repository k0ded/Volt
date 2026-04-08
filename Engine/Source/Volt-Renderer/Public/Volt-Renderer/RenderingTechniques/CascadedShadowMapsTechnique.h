#pragma once

#include "Volt-Renderer/GPUScene.h"

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>
#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	class CascadedShadowMapMeshProcessor;

	struct RenderView;
	struct RenderLightData;

	struct CascadedDirectionalShadowVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CascadedDirectionalShadowVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(CascadedDirectionalLightShadowMappingData, CascadedDirectionalLightShadowMapping)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
			SHADER_PARAMETER(uint, CascadeIndex)
		END_SHADER_PARAMETER_STRUCT()
	};

	struct CascadedDirectionalShadowPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CascadedDirectionalShadowPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};

	class CascadedShadowMapsTechnique
	{
	public:
		struct Result
		{
			RGTextureRef shadowMap;
			RGUniformBufferRef uniformBuffer;
		};

		CascadedShadowMapsTechnique(RenderGraph& renderGraph, CascadedShadowMapMeshProcessor* meshProcessor);
		Result Execute(const RenderView& view, const RenderLightData& renderLightData);

	private:
		RGUniformBufferRef UploadUniformBufferData(const RenderView& view, const RenderLightData& renderLightData);
		RGUniformBufferRef GenerateCascades(const RenderView& view, const RenderLightData& renderLightData);

		RenderGraph& m_renderGraph;
		CascadedShadowMapMeshProcessor* m_meshProcessor;
	};
}
