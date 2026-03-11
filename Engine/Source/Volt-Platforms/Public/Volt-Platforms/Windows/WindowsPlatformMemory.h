#pragma once

#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Config.h"
#include "Volt-Platforms/Fiber.h"

#include <CoreUtilities/Containers/Vector.h>

#include <cstdint>

namespace Volt
{
	class VTPL_API WindowsPlatformMemory
	{
	public:
		// Returns the base pointer (should be used to free the block)
		static void* AllocateFiberStacks(uint64_t stackSize, uint32_t numStacks, Vector<FiberStackDesc>& outStackBasePointers);
		static void FreeFiberStacks(void* baseAlloc);
	};
}

#endif
