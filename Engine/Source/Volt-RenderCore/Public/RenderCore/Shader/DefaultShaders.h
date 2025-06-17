#pragma once

#include "RenderCore/RenderGraph/ShaderParameterStruct.h"
#include "RenderCore/Shader/GlobalShader.h"

namespace Volt
{
	struct OpaqueDefaultPixelPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(OpaqueDefaultPixelPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};

	struct FullscreenTriangleVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(FullscreenTriangleVS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		END_SHADER_PARAMETER_STRUCT()
	};
}
