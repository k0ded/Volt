#pragma once

#include "RHIModule/RHICapabilities.h"

namespace Volt::RHI
{
	inline static bool RHICanUseRayTracing()
	{
		return g_rhiCapabilities.rayTracing.supportsRayTracing && g_rhiCapabilities.useRayTracing;
	}

	inline static bool RHICanUseMeshShaders()
	{
		return g_rhiCapabilities.supportsMeshShaders && g_rhiCapabilities.useMeshShaders;
	}

	inline static bool RHICanUseBindless()
	{
		return g_rhiCapabilities.supportsBindless && g_rhiCapabilities.useBindless;
	}
}
