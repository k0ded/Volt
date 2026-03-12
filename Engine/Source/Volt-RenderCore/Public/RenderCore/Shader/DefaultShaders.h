#pragma once

#include "RenderCore/RenderGraph/ShaderParameterStruct.h"
#include "RenderCore/Shader/GlobalShader.h"

namespace Volt
{
	struct FullscreenTriangleVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(FullscreenTriangleVS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		END_SHADER_PARAMETER_STRUCT()
	};
}
