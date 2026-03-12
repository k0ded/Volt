#pragma once

#include "Volt-Renderer/Material/MaterialShader.h"
#include "Volt-Renderer/Material/MaterialShaderRegistry.h"
#include "Volt-Renderer/GPUScene.h"

#include <RenderCore/RenderGraph/ShaderParameterStruct.h>

namespace Volt
{
	struct DefaultBasePassShaderPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DefaultBasePassShaderPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};

	struct DefaultDepthPrePassShaderPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DefaultDepthPrePassShaderPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};

	struct DefaultTranslucencyPassShaderPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DefaultTranslucencyPassShaderPS)
	};

	struct DepthPrePassVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DepthPrePassVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
		END_SHADER_PARAMETER_STRUCT()
	};

	struct BasePassVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(BasePassVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
		END_SHADER_PARAMETER_STRUCT()
	};

	struct TranslucencyPassVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(TranslucencyPassVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
		END_SHADER_PARAMETER_STRUCT()
	};

	struct DepthPrePassMaterialShader : public MaterialShader
	{
		VT_DECLARE_MATERIAL_SHADER(DepthPrePassMaterialShader);
	};

	struct BasePassMaterialShader : public MaterialShader
	{
		VT_DECLARE_MATERIAL_SHADER(BasePassMaterialShader);
	};

	struct TranslucencyPassMaterialShader : public MaterialShader
	{
		VT_DECLARE_MATERIAL_SHADER(TranslucencyPassMaterialShader);

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<int>, VisibleLightIndices)

			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, DFGLuT)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightIrradiance)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, SkylightRadiance)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2DArray<float>, CascadedDirectionalShadowMap)
			SHADER_PARAMETER_UNIFORM_BUFFER(CascadedDirectionalLightShadowMappingData, CascadedDirectionalLightShadowMapping)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
			SHADER_PARAMETER_SAMPLER(ShadowSampler)
			SHADER_PARAMETER(uint, NumRadianceMipLevels)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
}
