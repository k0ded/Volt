#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessors/DepthPrePassMeshProcessor.h"
#include "Volt-Renderer/SceneRendererShaderDefinitions.h"

#include <RenderCore/Shader/ShaderMap.h>

namespace Volt
{
	void DepthPrePassMeshProcessor::AddRenderPrimitive(const RenderPrimitiveData& renderPrimitive)
	{
		auto vertexShader = ShaderMap::Get<DepthPrePassVS>();
		auto pixelShader = ShaderMap::Get<DepthPrePassPS>();

		BuildMeshDrawCommand(renderPrimitive, {}, vertexShader, pixelShader);
	}

	void DepthPrePassMeshProcessor::RemoveRenderPrimitive(UUID64 renderPrimitveId)
	{
		RemoveMeshDrawCommand(renderPrimitveId);
	}
}
