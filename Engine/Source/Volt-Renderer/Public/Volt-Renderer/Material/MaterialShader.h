#pragma once

#include "Volt-Renderer/Material/MaterialCommon.h"

#include <RenderCore/Shader/PermutationCollection.h>
#include <RenderCore/Shader/GlobalShader.h>

namespace Volt
{
	struct MaterialShader : public GlobalShader
	{
		struct MaterialBlendModeDim : SHADER_PERMUTATION_ENUM("MATERIAL_BLEND_MODE", MaterialBlendMode);
		using PermutationVector = PermutationCollection<MaterialBlendModeDim>;
	};
}

#define VT_DECLARE_MATERIAL_SHADER(klass)

