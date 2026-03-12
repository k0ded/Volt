#pragma once

#include <Volt-Renderer/Debug/DebugMeshRenderer.h>
#include <Volt-Renderer/GPUScene.h>

#include <RenderCore/RenderGraph/ShaderParameterStruct.h>

struct ForwardLitDebugVS : public Volt::GlobalShader
{
	DECLARE_GLOBAL_SHADER(ForwardLitDebugVS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		RG_BUFFER_ACCESS(PrimitiveIndexVertexBuffer, Volt::RGResourceAccess::VertexBuffer)
		SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<DebugMeshData>, DebugMeshDatas)
		SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
	END_SHADER_PARAMETER_STRUCT()
};

struct ForwardLitDebugMaterialShader : public Volt::MaterialShader
{
	VT_DECLARE_MATERIAL_SHADER(ForwardLitDebugMaterialShader);

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
		SHADER_PARAMETER_STRUCT_INCLUDE(Volt::GPUSceneParameters, GPUScene)
		SHADER_PARAMETER_BUFFER_SRV(Buffer<int>, VisibleLightIndices)

		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SceneDepth)

		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, DFGLuT)
		SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightIrradiance)
		SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightRadiance)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2DArray<float>, CascadedDirectionalShadowMap)
		SHADER_PARAMETER_UNIFORM_BUFFER(CascadedDirectionalLightShadowMappingData, CascadedDirectionalLightShadowMapping)
		SHADER_PARAMETER_SAMPLER(LinearSampler)
		SHADER_PARAMETER_SAMPLER(ShadowSampler)
		SHADER_PARAMETER(uint, NumRadianceMipLevels)
	END_SHADER_PARAMETER_STRUCT()
};

struct TranslucencyDebugMaterialShader : public Volt::MaterialShader
{
	VT_DECLARE_MATERIAL_SHADER(TranslucencyDebugMaterialShader);

	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
		SHADER_PARAMETER_STRUCT_INCLUDE(Volt::GPUSceneParameters, GPUScene)
		SHADER_PARAMETER_BUFFER_SRV(Buffer<int>, VisibleLightIndices)

		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, SceneDepth)

		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, DFGLuT)
		SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightIrradiance)
		SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightRadiance)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2DArray<float>, CascadedDirectionalShadowMap)
		SHADER_PARAMETER_UNIFORM_BUFFER(CascadedDirectionalLightShadowMappingData, CascadedDirectionalLightShadowMapping)
		SHADER_PARAMETER_SAMPLER(LinearSampler)
		SHADER_PARAMETER_SAMPLER(ShadowSampler)
		SHADER_PARAMETER(uint, NumRadianceMipLevels)
	END_SHADER_PARAMETER_STRUCT()
};

class ForwardLitDebugMeshRenderer : public Volt::DebugMeshRenderer
{
public:
	void AddMeshDraw(Ref<Volt::Mesh> mesh, Ref<Volt::RenderMaterial> renderMaterial, const TQS& transform, const glm::vec4& userData) override;
	bool ShouldIncludeDraw(const Volt::RenderMaterial& renderMaterial) const override;
};

class TranslucencyDebugMeshRenderer : public Volt::DebugMeshRenderer
{
public:
	void AddMeshDraw(Ref<Volt::Mesh> mesh, Ref<Volt::RenderMaterial> renderMaterial, const TQS& transform, const glm::vec4& userData) override;
	bool ShouldIncludeDraw(const Volt::RenderMaterial& renderMaterial) const override;
};
