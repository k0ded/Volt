#pragma once

#ifdef VT_PLATFORM_WINDOWS

#include "PlatformsModule/Config.h"
#include "PlatformsModule/ThreadPriority.h"

#include <CoreUtilities/Time/Time.h>
#include <CoreUtilities/String/StringView.h>

#include <thread>
#include <chrono>

namespace Volt
{
	class VTPL_API WindowsPlatformThread
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void SetThreadName(std::thread::native_handle_type threadHandle, StringView threadName);
		static void SetThreadPriority(std::thread::native_handle_type threadHandle, ThreadPriority priority);
		static void AssignThreadToCore(std::thread::native_handle_type threadHandle, uint64_t affinityMask);
		static std::thread::native_handle_type GetMainThreadHandle() { return m_mainThreadHandle; }
		static std::thread::native_handle_type GetCurrentThreadHandle();

		template<typename Period = Time::Milliseconds>
		static void Sleep(const float duration)
		{
			auto chronoDuration = std::chrono::duration<float, Period>(duration);

			if constexpr (!std::is_same_v<Period, std::chrono::milliseconds::period>)
			{
				auto durationInMilli = std::chrono::duration<float, std::chrono::milliseconds::period>(chronoDuration);
				SleepInternal(durationInMilli.count());
			}
			else
			{
				SleepInternal(chronoDuration.count());
			}
		}

	private:
		static void SleepInternal(const float durationInMilliseconds);

		static std::thread::native_handle_type m_mainThreadHandle;
	};
}

#endif
