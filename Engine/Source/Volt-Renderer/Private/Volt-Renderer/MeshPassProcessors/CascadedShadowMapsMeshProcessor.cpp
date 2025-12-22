#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessors/CascadedShadowMapsMeshProcessor.h"
#include "Volt-Renderer/RenderingTechniques/CascadedShadowMapsTechnique.h"

#include <RenderCore/Shader/ShaderMap.h>

namespace Volt
{
	void CascadedShadowMapMeshProcessor::AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		auto vertexShader = ShaderMap::Get<CascadedDirectionalShadowVS>();
		auto pixelShader = ShaderMap::Get<CascadedDirectionalShadowPS>();

		RHI::RenderPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.depthCompareOperator = RHI::CompareOperator::LessEqual;
		pipelineCreateInfo.enableDepthClamp = true;
		pipelineCreateInfo.depthBiasClamp = 1.f / 128.f;
		pipelineCreateInfo.depthBiasSlopeFactor = 3.f;

		BuildMeshDrawCommand(renderPrimitive, pipelineCreateInfo, vertexShader, pixelShader);
	}

	void CascadedShadowMapMeshProcessor::RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		RemoveMeshDrawCommand(renderPrimitive);
	}
}
