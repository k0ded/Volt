#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Config.h"

#include "Volt-Platforms/Windows/WindowsPlatformMutex.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>

namespace Volt
{
	WindowsPlatformMutex::WindowsPlatformMutex()
	{
		// Make sure that the sizes always match.
		static_assert(sizeof(CriticalSection) == sizeof(::CRITICAL_SECTION));

		::InitializeCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_criticalSection));
		::SetCriticalSectionSpinCount(reinterpret_cast<LPCRITICAL_SECTION>(&m_criticalSection), 4000);
	}

	WindowsPlatformMutex::~WindowsPlatformMutex()
	{
		::DeleteCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_criticalSection));
	}

	void WindowsPlatformMutex::lock()
	{
		::EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_criticalSection));
	}

	void WindowsPlatformMutex::unlock()
	{
		::LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_criticalSection));
	}

	bool WindowsPlatformMutex::try_lock()
	{
		return !!::TryEnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_criticalSection));
	}
}

#endif
