#pragma once

#include "RHIModule/Core/Core.h"

namespace Volt::RHI
{
	struct RHIConfiguration
	{
		bool useMeshShaders = false;
		bool useBindless = false;
		bool useRayTracing = false;
	};
}

extern VTRHI_API Volt::RHI::RHIConfiguration g_rhiConfiguration;
