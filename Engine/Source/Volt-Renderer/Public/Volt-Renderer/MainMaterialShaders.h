#pragma once

#include "Volt-Renderer/Material/MaterialShader.h"
#include "Volt-Renderer/Material/MaterialShaderRegistry.h"

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

	struct DepthPrePassMaterialShader : public MaterialShader
	{
		VT_DECLARE_MATERIAL_SHADER(DepthPrePassMaterialShader);
	};

	struct BasePassMaterialShader : public MaterialShader
	{
		VT_DECLARE_MATERIAL_SHADER(BasePassMaterialShader);
	};
}
