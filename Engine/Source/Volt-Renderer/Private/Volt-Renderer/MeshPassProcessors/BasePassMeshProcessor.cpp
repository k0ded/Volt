#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessors/BasePassMeshProcessor.h"
#include "Volt-Renderer/Material/RenderMaterial.h"
#include "Volt-Renderer/RenderPrimitiveData.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/MainMaterialShaders.h"

#include <RenderCore/Shader/ShaderMap.h>

namespace Volt
{
	void BasePassMeshProcessor::AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		if (!ShouldIncludePrimitive(renderPrimitive))
		{
			return;
		}

		auto vertexShader = ShaderMap::Get<BasePassVS>();
		auto pixelShader = renderPrimitive->material->GetPixelShader<BasePassMaterialShader>();

		BuildMeshDrawCommand(renderPrimitive, {}, vertexShader, pixelShader);
	}

	void BasePassMeshProcessor::RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		if (!ShouldIncludePrimitive(renderPrimitive))
		{
			return;
		}

		RemoveMeshDrawCommand(renderPrimitive);
	}

	bool BasePassMeshProcessor::ShouldIncludePrimitive(const RenderPrimitiveData* renderPrimitive) const
	{
		const MaterialBlendMode materialBlendMode = renderPrimitive->material->GetMaterialBlendMode();

		return materialBlendMode == MaterialBlendMode::Opaque ||
			materialBlendMode == MaterialBlendMode::AlphaMasked;
	}
}
