#pragma once

#include "CoreUtilities/Core.h"

#include <chrono>

namespace TimeUtility
{
	template<typename CLOCK = std::chrono::system_clock>
	VT_NODISCARD VT_INLINE uint64_t GetTimeSinceEpoch()
	{
		auto duration = CLOCK::now().time_since_epoch();
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

		return static_cast<uint64_t>(millis);
	}
}
