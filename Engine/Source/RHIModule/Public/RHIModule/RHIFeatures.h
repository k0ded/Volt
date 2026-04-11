#pragma once

#include "RHIModule/RHICapabilities.h"
#include "RHIModule/RHIConfiguration.h"

namespace Volt::RHI
{
	inline static bool RHICanUseRayTracing()
	{
		return g_rhiCapabilities.rayTracing.supportsRayTracing && g_rhiConfiguration.useRayTracing;
	}

	inline static bool RHICanUseMeshShaders()
	{
		return g_rhiCapabilities.supportsMeshShaders && g_rhiConfiguration.useMeshShaders;
	}

	inline static bool RHICanUseBindless()
	{
		return g_rhiCapabilities.supportsBindless && g_rhiConfiguration.useBindless;
	}
}
