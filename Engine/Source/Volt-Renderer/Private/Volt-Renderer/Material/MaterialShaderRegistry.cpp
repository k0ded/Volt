#include "vrpch.h"

#include "Volt-Renderer/Material/MaterialShaderRegistry.h"

namespace Volt
{
	MaterialShaderRegistry& MaterialShaderRegistry::Get()
	{
		static MaterialShaderRegistry registry;
		return registry;
	}
}
