#include "sbpch.h"

#include "Sandbox/SceneRendererExtensions/ObjectIDPassMeshProcessor.h"
#include "Sandbox/SceneRendererExtensions/ObjectIDSceneRendererExtension.h"

#include <RenderCore/Shader/ShaderMap.h>

#include <RHIModule/Pipelines/RenderPipeline.h>

void ObjectIDPassMeshProcessor::AddRenderPrimitive(const Volt::RenderPrimitiveData& renderPrimitive)
{
	auto vertexShader = Volt::ShaderMap::Get<ObjectIDVS>();
	auto pixelShader = Volt::ShaderMap::Get<ObjectIDPS>();

	Volt::RHI::RenderPipelineCreateInfo pipelineInfo;
	pipelineInfo.depthMode = Volt::RHI::DepthMode::Read;

	BuildMeshDrawCommand(renderPrimitive, pipelineInfo, vertexShader, pixelShader);
}

void ObjectIDPassMeshProcessor::RemoveRenderPrimitive(const Volt::RenderPrimitiveData& renderPrimitive)
{
	RemoveMeshDrawCommand(renderPrimitive);
}

