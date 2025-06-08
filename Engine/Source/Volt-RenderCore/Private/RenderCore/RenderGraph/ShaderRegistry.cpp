#include "rcpch.h"

#include "RenderCore/RenderGraph/ShaderRegistry.h"

#include <RHIModule/Shader/ShaderCommon.h>

Volt::ShaderRegistry g_shaderRegistry;

namespace Volt
{
	ShaderRegistry& ShaderRegistry::Get()
	{
		return g_shaderRegistry;
	}
}
