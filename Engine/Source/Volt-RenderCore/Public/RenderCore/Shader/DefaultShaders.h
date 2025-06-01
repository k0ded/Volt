#pragma once

#include "RenderCore/RenderGraph/ShaderRegistryMacros.h"
#include "RenderCore/Shader/GlobalShader.h"

namespace Volt
{
#if 0
	struct OpaqueDefaultMaterialCS
	{
		BEGIN_SHADER_DEFINITION(OpaqueDefaultMaterialCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Defaults/OpaqueDefault_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		END_SHADER_PARAMETER_STRUCT()
	};
#endif

	struct FullscreenTriangleVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(FullscreenTriangleVS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		END_SHADER_PARAMETER_STRUCT()
	};
}
