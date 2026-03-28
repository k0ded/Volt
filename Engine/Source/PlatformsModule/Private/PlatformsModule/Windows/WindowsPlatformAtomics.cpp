#include "PlatformsModule/Windows/WindowsPlatformAtomics.h"

#ifdef VT_PLATFORM_WINDOWS

#include <CoreUtilities/Platform/Windows/VoltWindows.h>

#undef InterlockedCompareExchange
#undef InterlockedExchange
#undef InterlockedIncrement
#undef InterlockedDecrement

namespace Volt
{
	long WindowsPlatformAtomics::InterlockedCompareExchange(long volatile* destination, long exchange, long comperand)
	{
		return ::_InterlockedCompareExchange(destination, exchange, comperand);
	}

	long WindowsPlatformAtomics::InterlockedExchange(long volatile* destination, long value)
	{
		return ::_InterlockedExchange(destination, value);
	}

	long WindowsPlatformAtomics::InterlockedIncrement(long volatile* destination)
	{
		return ::_InterlockedIncrement(destination);
	}

	long WindowsPlatformAtomics::InterlockedDecrement(long volatile* destination)
	{
		return ::_InterlockedDecrement(destination);
	}
}
#endif
