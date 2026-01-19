#pragma once

#include "Volt-Renderer/Material/MaterialShader.h"
#include "Volt-Renderer/Material/MaterialShaderRegistry.h"

namespace Volt
{
	struct DepthPrePassMaterialShader : public MaterialShader
	{
		VT_DECLARE_MATERIAL_SHADER(DepthPrePassMaterialShader);
	};

	struct BasePassMaterialShader : public MaterialShader
	{
		VT_DECLARE_MATERIAL_SHADER(BasePassMaterialShader);
	};
}
