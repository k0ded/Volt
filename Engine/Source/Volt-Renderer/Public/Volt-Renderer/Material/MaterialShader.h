#pragma once

#include "Volt-Renderer/Material/MaterialCommon.h"

#include <RenderCore/Shader/PermutationCollection.h>
#include <RenderCore/Shader/GlobalShader.h>

namespace Volt
{
	struct MaterialShader : public GlobalShader
	{
		struct InlineParameterBlock
		{
			uint32_t materialBlendMode;
			uint32_t isDoubleSided;
		};
	};
}

#define VT_DECLARE_MATERIAL_SHADER(klass)
