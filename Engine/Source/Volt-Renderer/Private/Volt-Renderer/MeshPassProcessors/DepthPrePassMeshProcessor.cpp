#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessors/DepthPrePassMeshProcessor.h"
#include "Volt-Renderer/SceneRendererShaderDefinitions.h"
#include "Volt-Renderer/Material/RenderMaterial.h"
#include "Volt-Renderer/MainMaterialShaders.h"
#include "Volt-Renderer/RenderPrimitiveData.h"

#include <RenderCore/Shader/ShaderMap.h>

namespace Volt
{
	void DepthPrePassMeshProcessor::AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		auto vertexShader = ShaderMap::Get<DepthPrePassVS>();
		auto pixelShader = renderPrimitive->material->GetPixelShader<DepthPrePassMaterialShader>();

		BuildMeshDrawCommand(renderPrimitive, {}, vertexShader, pixelShader);
	}

	void DepthPrePassMeshProcessor::RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		RemoveMeshDrawCommand(renderPrimitive);
	}
}
