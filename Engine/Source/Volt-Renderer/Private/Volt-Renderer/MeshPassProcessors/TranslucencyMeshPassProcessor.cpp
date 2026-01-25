#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessors/TranslucencyMeshPassProcessor.h"
#include "Volt-Renderer/Material/RenderMaterial.h"
#include "Volt-Renderer/RenderPrimitiveData.h"
#include "Volt-Renderer/MainMaterialShaders.h"

#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/DefaultBlendStates.h>

namespace Volt
{
	void TranslucencyMeshPassProcessor::AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		if (!ShouldIncludePrimitive(renderPrimitive))
		{
			return;
		}

		auto vertexShader = ShaderMap::Get<TranslucencyPassVS>();
		auto pixelShader = renderPrimitive->material->GetPixelShader<TranslucencyPassMaterialShader>();

		// Enable alpha blending for attachment 0 (accumulation)
		RHI::RenderPipelineCreateInfo renderPipelineInfo;
		renderPipelineInfo.attachmentBlendStates[0] = DefaultBlendStates::Add();
		renderPipelineInfo.attachmentBlendStates[1] = DefaultBlendStates::OneMinusSrcColor();

		BuildMeshDrawCommand(renderPrimitive, renderPipelineInfo, vertexShader, pixelShader);
	}

	void TranslucencyMeshPassProcessor::RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		if (!ShouldIncludePrimitive(renderPrimitive))
		{
			return;
		}
	}
	
	bool TranslucencyMeshPassProcessor::ShouldIncludePrimitive(const RenderPrimitiveData* renderPrimitive) const
	{
		const MaterialBlendMode materialBlendMode = renderPrimitive->material->GetMaterialBlendMode();
		return materialBlendMode == MaterialBlendMode::Translucent;
	}
}
