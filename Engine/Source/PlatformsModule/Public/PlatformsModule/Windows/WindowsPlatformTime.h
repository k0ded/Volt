#pragma once

#ifdef VT_PLATFORM_WINDOWS

#include "PlatformsModule/Config.h"

#include <cstdint>

namespace Volt
{
	class VTPL_API WindowsPlatformTime
	{
	public:
		static void Initialize();

		static uint32_t GetTime();
		static uint64_t GetTime64();
	
		static float ToMilliseconds(const uint32_t time);
		static float ToSeconds(const uint32_t time);

		static double ToMilliseconds(const uint64_t time);
		static double ToSeconds(const uint64_t time);

	private:
		inline static double m_secondsPerCycle = 0.0;
	};
}
#endif
