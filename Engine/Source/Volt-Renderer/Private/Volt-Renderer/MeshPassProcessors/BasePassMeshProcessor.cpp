#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessors/BasePassMeshProcessor.h"
#include "Volt-Renderer/Material/RenderMaterial.h"
#include "Volt-Renderer/RenderPrimitiveData.h"
#include "Volt-Renderer/SceneRendererShaderDefinitions.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/MainMaterialShaders.h"

#include <RenderCore/Shader/ShaderMap.h>

namespace Volt
{
	void BasePassMeshProcessor::AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		auto vertexShader = ShaderMap::Get<BasePassVS>();
		auto pixelShader = renderPrimitive->material->GetPixelShader<BasePassMaterialShader>();

		if (!pixelShader)
		{
			pixelShader = Renderer::GetDefaultResources().defaultMaterial->GetPixelShader<BasePassMaterialShader>();
		}

		BuildMeshDrawCommand(renderPrimitive, {}, vertexShader, pixelShader);
	}

	void BasePassMeshProcessor::RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		RemoveMeshDrawCommand(renderPrimitive);
	}
}
