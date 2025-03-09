#pragma once

#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>

namespace Volt
{
	struct OpaqueDefaultMaterialCS
	{
		BEGIN_SHADER_DEFINITION(OpaqueDefaultMaterialCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Defaults/OpaqueDefault_cs.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()
	};
}
