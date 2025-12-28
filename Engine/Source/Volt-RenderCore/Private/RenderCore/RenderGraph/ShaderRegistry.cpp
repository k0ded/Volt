#include "rcpch.h"

#include "RenderCore/RenderGraph/ShaderRegistry.h"

#include <RHIModule/Shader/ShaderCommon.h>

namespace Volt
{
	ShaderRegistry& ShaderRegistry::Get()
	{
		static ShaderRegistry registry;
		return registry;
	}
}
