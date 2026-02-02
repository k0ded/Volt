#include "sbpch.h"

#include "Sandbox/DebugMeshRenderers/DebugMeshRenderers.h"

#include <Volt-Renderer/Material/MaterialShaderRegistry.h>

#include <RenderCore/RenderGraph/ShaderRegistry.h>
#include <RenderCore/Shader/ShaderMap.h>

using namespace Volt;

struct DefaultForwardLitDebugShaderPS : public GlobalShader
{
	DECLARE_GLOBAL_SHADER(DefaultForwardLitDebugShaderPS)
};
VT_REGISTER_SHADER(DefaultForwardLitDebugShaderPS, "Engine/Shaders/Source/Debug/DefaultForwardLitDebugShaderPS.hlsl", "MainPS", Pixel);
VT_REGISTER_SHADER(ForwardLitDebugVS, "Engine/Shaders/Source/Debug/ForwardLitDebugVS.hlsl", "MainVS", Vertex);

VT_REGISTER_MATERIAL_SHADER(ForwardLitDebugMaterialShader, DefaultForwardLitDebugShaderPS, "Engine/Shaders/Source/Debug/ForwardLitDebugPixel.hlsl", "MainPS");

void ForwardLitDebugMeshRenderer::AddMeshDraw(Ref<Mesh> mesh, Ref<RenderMaterial> renderMaterial, const TQS& transform, uint32_t userData)
{
	auto vertexShader = ShaderMap::Get<ForwardLitDebugVS>();
	auto pixelShader = renderMaterial->GetPixelShader<ForwardLitDebugMaterialShader>();

	RHI::RenderPipelineCreateInfo pipelineInfo{};
	if (renderMaterial->GetIsDoubleSided())
	{
		pipelineInfo.cullMode = RHI::CullMode::None;
	}

	BuildMeshDrawCommand(mesh, renderMaterial, transform, userData, pipelineInfo, vertexShader, pixelShader);
}

bool ForwardLitDebugMeshRenderer::ShouldIncludeDraw(const RenderMaterial& renderMaterial) const
{
	const MaterialBlendMode materialBlendMode = renderMaterial.GetMaterialBlendMode();
	return materialBlendMode == MaterialBlendMode::Opaque ||
		materialBlendMode == MaterialBlendMode::AlphaMasked;
}
