#pragma once

#ifdef VT_PLATFORM_WINDOWS

#include "Volt-Platforms/Config.h"

namespace Volt
{
	// Follow std lib code standard to be able to use in std::scoped_lock etc.
	class VTPL_API WindowsPlatformMutex
	{
	public:
		WindowsPlatformMutex();
		~WindowsPlatformMutex();

		void lock();
		void unlock();
		bool try_lock();

	private:
		// A copy of Windows CRITICAL_SECTION to avoid including it.
		struct CriticalSection
		{
			void* DebugInfo;

			long LockCount;
			long RecursionCount;
			void* OwningThread;        // from the thread's ClientId->UniqueThread
			void* LockSemaphore;
			void* SpinCount;        // force size on 64-bit systems when packed
		};

		CriticalSection m_criticalSection;
	};
}

#endif
