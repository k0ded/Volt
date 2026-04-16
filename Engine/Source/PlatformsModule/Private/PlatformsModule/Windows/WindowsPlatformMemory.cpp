#include "PlatformsModule/Windows/WindowsPlatformMemory.h"

#ifdef VT_PLATFORM_WINDOWS

#include <CoreUtilities/Platform/Windows/VoltWindows.h>

namespace Volt
{
	void* WindowsPlatformMemory::AllocateFiberStacks(uint64_t stackSize, uint32_t numStacks, Vector<FiberStackDesc>& outStackBasePointers)
	{
		// #Note_Ivar: Is this always the same size?
		constexpr uint64_t GuardSize = 0x1000;

		const uint64_t totalAllocationSize = (stackSize + GuardSize) * numStacks;

		uint8_t* baseAlloc = reinterpret_cast<uint8_t*>(VirtualAlloc(
			nullptr,
			totalAllocationSize,
			MEM_RESERVE | MEM_COMMIT,
			PAGE_READWRITE
		));

		VT_ENSURE(baseAlloc != nullptr);

		outStackBasePointers.resize_uninitialized(numStacks);

		for (uint32_t i = 0; i < numStacks; ++i)
		{
			// Guard at the beginning of each "allocation"
			uint8_t* guardPtr = baseAlloc + (stackSize + GuardSize) * i;
			// Stack base lies after the guard page.
			uint8_t* stackBasePtr = baseAlloc + (stackSize + GuardSize) * i + GuardSize;

			outStackBasePointers[i].guardBase = guardPtr;
			outStackBasePointers[i].stackBase = stackBasePtr;

			DWORD old;
			VT_MAYBE_UNUSED BOOL ok = VirtualProtect(guardPtr, GuardSize, PAGE_READWRITE | PAGE_GUARD, &old);
			VT_ENSURE(ok);
		}

		return baseAlloc;
	}


	void WindowsPlatformMemory::FreeFiberStacks(void* baseAlloc)
	{
		VirtualFree(baseAlloc, 0, MEM_RELEASE);
	}
}
#endif
