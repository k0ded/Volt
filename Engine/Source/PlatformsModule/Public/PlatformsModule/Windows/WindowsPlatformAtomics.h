#pragma once

#ifdef VT_PLATFORM_WINDOWS

#include "PlatformsModule/Config.h"

namespace Volt
{
	class VTPL_API WindowsPlatformAtomics
	{
	public:
		static long InterlockedCompareExchange(long volatile* destination, long exchange, long comperand);
		static long InterlockedExchange(long volatile* destination, long value);
		static long InterlockedIncrement(long volatile* destination);
		static long InterlockedDecrement(long volatile* destination);
	};
}
#endif
