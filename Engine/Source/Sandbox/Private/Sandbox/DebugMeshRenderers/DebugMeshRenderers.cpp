#include "sbpch.h"

#include "Sandbox/DebugMeshRenderers/DebugMeshRenderers.h"
#include "Sandbox/ComponentVisualizers/EditorDrawInterfaceUserData.h"

#include <Volt-Renderer/Material/MaterialShaderRegistry.h>

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/DefaultBlendStates.h>

using namespace Volt;

struct DefaultForwardLitDebugShaderPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DefaultForwardLitDebugShaderPS)
};

struct DefaultTranslucencyDebugShaderPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DefaultTranslucencyDebugShaderPS)
};

VT_REGISTER_SHADER(DefaultForwardLitDebugShaderPS, "Engine/Shaders/Source/Debug/DefaultForwardLitDebugShaderPS.hlsl", "MainPS", Pixel);
VT_REGISTER_SHADER(DefaultTranslucencyDebugShaderPS, "Engine/Shaders/Source/Debug/DefaultTranslucencyDebugShaderPS.hlsl", "MainPS", Pixel);
VT_REGISTER_SHADER(ForwardLitDebugVS, "Engine/Shaders/Source/Debug/ForwardLitDebugVS.hlsl", "MainVS", Vertex);

VT_REGISTER_MATERIAL_SHADER(ForwardLitDebugMaterialShader, DefaultForwardLitDebugShaderPS, "Engine/Shaders/Source/Debug/ForwardLitDebugPixel.hlsl", "MainPS");
VT_REGISTER_MATERIAL_SHADER(TranslucencyDebugMaterialShader, DefaultTranslucencyDebugShaderPS, "Engine/Shaders/Source/Debug/TranslucencyDebugPixel.hlsl", "MainPS");

void ApplyDebugRenderingLayerSettings(RHI::RenderPipelineCreateInfo& pipelineInfo, DebugRenderingLayer layer)
{
	// If the mesh should be rendered in the foreground or foreground world, no depth info should be used.
	if (layer == DebugRenderingLayer::Foreground ||
		layer == DebugRenderingLayer::ForegroundWorld)
	{
		pipelineInfo.depthMode = RHI::DepthMode::None;
	}
}

void ForwardLitDebugMeshRenderer::AddMeshDraw(Ref<Mesh> mesh, Ref<RenderMaterial> renderMaterial, const TQS& transform, const glm::vec4& userData)
{
	const EditorDrawInterfaceUserData unpackedUserData = EditorDrawInterfaceUserData::Unpack(userData);

	auto vertexShader = ShaderMap::Get<ForwardLitDebugVS>();
	auto pixelShader = renderMaterial->GetPixelShader<ForwardLitDebugMaterialShader>();

	RHI::RenderPipelineCreateInfo pipelineInfo{};
	if (renderMaterial->GetIsDoubleSided())
	{
		pipelineInfo.cullMode = RHI::CullMode::None;
	}

	ApplyDebugRenderingLayerSettings(pipelineInfo, unpackedUserData.layer);
	BuildMeshDrawCommand(mesh, renderMaterial, transform, userData, pipelineInfo, vertexShader, pixelShader);
}

bool ForwardLitDebugMeshRenderer::ShouldIncludeDraw(const RenderMaterial& renderMaterial) const
{
	const MaterialBlendMode materialBlendMode = renderMaterial.GetMaterialBlendMode();
	return materialBlendMode == MaterialBlendMode::Opaque ||
		materialBlendMode == MaterialBlendMode::AlphaMasked;
}

void TranslucencyDebugMeshRenderer::AddMeshDraw(Ref<Mesh> mesh, Ref<RenderMaterial> renderMaterial, const TQS& transform, const glm::vec4& userData)
{
	const EditorDrawInterfaceUserData unpackedUserData = EditorDrawInterfaceUserData::Unpack(userData);

	auto vertexShader = ShaderMap::Get<ForwardLitDebugVS>();
	auto pixelShader = renderMaterial->GetPixelShader<TranslucencyDebugMaterialShader>();

	RHI::RenderPipelineCreateInfo pipelineInfo{};
	pipelineInfo.depthMode = RHI::DepthMode::Read;
	pipelineInfo.attachmentBlendStates[0] = Volt::DefaultBlendStates::Add();
	pipelineInfo.attachmentBlendStates[1] = Volt::DefaultBlendStates::OneMinusSrcColor();

	if (renderMaterial->GetIsDoubleSided())
	{
		pipelineInfo.cullMode = RHI::CullMode::None;
	}

	ApplyDebugRenderingLayerSettings(pipelineInfo, unpackedUserData.layer);
	BuildMeshDrawCommand(mesh, renderMaterial, transform, userData, pipelineInfo, vertexShader, pixelShader);
}

bool TranslucencyDebugMeshRenderer::ShouldIncludeDraw(const RenderMaterial& renderMaterial) const
{
	const MaterialBlendMode materialBlendMode = renderMaterial.GetMaterialBlendMode();
	return materialBlendMode == MaterialBlendMode::Translucent;
}
