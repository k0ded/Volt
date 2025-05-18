#ifdef VT_PLATFORM_WINDOWS
#include "Volt-Platforms/Windows/WindowsPlatformThread.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/StringUtility.h>

namespace Volt
{
	void WindowsPlatformThread::SetThreadName(std::thread::native_handle_type threadHandle, std::string_view threadName)
	{
		std::wstring wThreadName = Utility::ToWString(threadName);
		HRESULT hr = SetThreadDescription(threadHandle, wThreadName.c_str());
		VT_UNUSED(hr);
		VT_ASSERT(SUCCEEDED(hr));
	}
	
	void WindowsPlatformThread::SetThreadPriority(std::thread::native_handle_type threadHandle, ThreadPriority priority)
	{
		int32_t threadPriority = 0;
		switch (priority)
		{
			case ThreadPriority::High: threadPriority = THREAD_PRIORITY_HIGHEST; break;
			case ThreadPriority::Medium: threadPriority = THREAD_PRIORITY_NORMAL; break;
			case ThreadPriority::Low: threadPriority = THREAD_PRIORITY_LOWEST; break;
		}

		BOOL priorityResult = ::SetThreadPriority(threadHandle, threadPriority);
		VT_UNUSED(priorityResult);
		VT_ASSERT(priorityResult);
	}
	
	void WindowsPlatformThread::AssignThreadToCore(std::thread::native_handle_type threadHandle, uint64_t affinityMask)
	{
		DWORD_PTR result = SetThreadAffinityMask(threadHandle, affinityMask);
		VT_UNUSED(result);
		VT_ASSERT(result > 0);
	}

	void WindowsPlatformThread::SleepInternal(const float durationInMilliseconds)
	{
		std::this_thread::sleep_for(std::chrono::duration<float, std::chrono::milliseconds::period>(durationInMilliseconds));
	}

	std::thread::native_handle_type WindowsPlatformThread::GetCurrentThreadHandle()
	{
		HANDLE threadHandle = ::GetCurrentThread();
		return reinterpret_cast<std::thread::native_handle_type>(threadHandle);
	}
}
#endif
