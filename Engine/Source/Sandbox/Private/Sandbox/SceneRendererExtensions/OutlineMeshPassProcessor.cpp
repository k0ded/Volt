#include "sbpch.h"

#include "Sandbox/SceneRendererExtensions/OutlineMeshPassProcessor.h"
#include "Sandbox/SceneRendererExtensions/OutlineTechnique.h"

#include <RenderCore/Shader/ShaderMap.h>

#include <RHIModule/Pipelines/RenderPipeline.h>

void OutlineMeshPassProcessor::AddRenderPrimitive(const Volt::RenderPrimitiveData* renderPrimitive)
{
	auto vertexShader = Volt::ShaderMap::Get<OutlineGeometryVS>();
	auto pixelShader = Volt::ShaderMap::Get<OutlineGeometryPS>();

	Volt::RHI::RenderPipelineCreateInfo pipelineInfo;
	pipelineInfo.depthMode = Volt::RHI::DepthMode::Read;

	BuildMeshDrawCommand(renderPrimitive, pipelineInfo, vertexShader, pixelShader);
}

void OutlineMeshPassProcessor::RemoveRenderPrimitive(const Volt::RenderPrimitiveData* renderPrimitive)
{
	RemoveMeshDrawCommand(renderPrimitive);
}
