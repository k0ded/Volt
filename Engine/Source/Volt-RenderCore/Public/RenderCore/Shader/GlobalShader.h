#pragma once

#include "RenderCore/RenderGraph/ShaderRegistryMacros.h"

namespace Volt
{
	struct GlobalShaderPermutationParameters
	{
		size_t permutationIndex;
	};

	// Base class for all Volt engine shaders.
	class GlobalShader
	{
	public:
	};
}
