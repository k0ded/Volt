#include "sbpch.h"

#include "Sandbox/SceneRendererExtensions/DebugSceneRendererExtension.h"
#include "Sandbox/SceneRendererExtensions/ObjectIDSceneRendererExtension.h"

#include <Volt-Renderer/Debug/DebugRenderer.h>
#include <Volt-Renderer/SceneRendererRenderGraphData.h>

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/Shader/ShaderMap.h>

struct EditorGizmoPS : public Volt::GlobalShader
{
	DECLARE_GLOBAL_SHADER(EditorGizmoPS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
	END_SHADER_PARAMETER_STRUCT()
};
VT_REGISTER_SHADER(EditorGizmoPS, "Engine/Shaders/Source/Editor/EditorGizmoPS.hlsl", "MainPS", Pixel);

DebugSceneRendererExtension::DebugSceneRendererExtension(Ref<Volt::RenderScene> renderScene, Volt::DebugRenderer& debugRenderer)
	: Volt::SceneRendererExtension(renderScene),
	m_debugRenderer(debugRenderer)
{}

Volt::RGTextureRef DebugSceneRendererExtension::OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage)
{
	const Volt::SceneTextures& sceneTextures = blackboard.Get<Volt::SceneTextures>();
	const ObjectIDTexture& objectIdTexture = blackboard.Get<ObjectIDTexture>();

	auto pixelShader = Volt::ShaderMap::Get<EditorGizmoPS>();

	Volt::ShaderParameterRenderTargetBindings renderTargets;
	renderTargets.renderTargets[0] = prevOutputImage;
	renderTargets.renderTargets[1] = objectIdTexture.texture;
	renderTargets.depthTarget = sceneTextures.sceneDepth;

	m_debugRenderer.RenderBillboards(renderGraph, pixelShader, view, renderTargets, false);

	return prevOutputImage;
}
