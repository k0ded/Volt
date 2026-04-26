#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessors/DepthPrePassMeshProcessor.h"
#include "Volt-Renderer/Material/RenderMaterial.h"
#include "Volt-Renderer/MainMaterialShaders.h"
#include "Volt-Renderer/RenderPrimitiveData.h"

#include <RenderCore/Shader/GlobalShaderMap.h>

namespace Volt
{
	void DepthPrePassMeshProcessor::AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		// Skip translucency.
		if (!ShouldIncludePrimitive(renderPrimitive))
		{
			return;
		}

		auto vertexShader = GlobalShaderMap::Get<DepthPrePassVS>();
		auto pixelShader = renderPrimitive->material->GetPixelShader<DepthPrePassMaterialShader>();

		RHI::RenderPipelineCreateInfo pipelineInfo{};
		if (renderPrimitive->material->GetIsDoubleSided())
		{
			pipelineInfo.cullMode = RHI::CullMode::None;
		}

		BuildMeshDrawCommand(renderPrimitive, pipelineInfo, vertexShader, pixelShader);
	}

	void DepthPrePassMeshProcessor::RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		if (!ShouldIncludePrimitive(renderPrimitive))
		{
			return;
		}

		RemoveMeshDrawCommand(renderPrimitive);
	}

	bool DepthPrePassMeshProcessor::ShouldIncludePrimitive(const RenderPrimitiveData* renderPrimitive) const
	{
		const MaterialBlendMode materialBlendMode = renderPrimitive->material->GetMaterialBlendMode();
	
		return materialBlendMode == MaterialBlendMode::Opaque ||
			materialBlendMode == MaterialBlendMode::AlphaMasked;
	}
}
