#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Windows/WindowsPlatformTime.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>

namespace Volt
{
	void WindowsPlatformTime::Initialize()
	{
		::LARGE_INTEGER frequency;
		::QueryPerformanceFrequency(&frequency);

		m_secondsPerCycle = 1.0 / static_cast<double>(frequency.QuadPart);
	}

	uint32_t WindowsPlatformTime::GetTime()
	{
		::LARGE_INTEGER counter;
		::QueryPerformanceCounter(&counter);

		return static_cast<uint32_t>(counter.QuadPart);
	}

	uint64_t WindowsPlatformTime::GetTime64()
	{
		::LARGE_INTEGER counter;
		::QueryPerformanceCounter(&counter);

		return counter.QuadPart;
	}

	float WindowsPlatformTime::ToMilliseconds(const uint32_t time)
	{
		return static_cast<float>(double(m_secondsPerCycle * 1000.0 * time));
	}

	float WindowsPlatformTime::ToSeconds(const uint32_t time)
	{
		return static_cast<float>(double(m_secondsPerCycle * time));
	}

	double WindowsPlatformTime::ToMilliseconds(const uint64_t time)
	{
		return double(m_secondsPerCycle * 1000.0 * time);
	}

	double WindowsPlatformTime::ToSeconds(const uint64_t time)
	{
		return double(m_secondsPerCycle * time);
	}


}

#endif
