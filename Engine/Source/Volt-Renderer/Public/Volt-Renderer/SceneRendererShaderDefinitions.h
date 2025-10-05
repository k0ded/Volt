#pragma once

#include "Volt-Renderer/GPUScene.h"

#include <RenderCore/Shader/GlobalShader.h>
#include <RenderCore/RenderGraph/ShaderRegistry.h>

namespace Volt
{
	struct DepthPrePassVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DepthPrePassVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
		END_SHADER_PARAMETER_STRUCT()
	};

	struct DepthPrePassPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(DepthPrePassPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ViewData, View)
			RG_RENDER_TARGETS()
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

	// Note: Dummy base pass to supply render targets.
	struct BasePassPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(BasePassPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
}
