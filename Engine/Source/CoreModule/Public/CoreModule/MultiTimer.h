#pragma once

#include "CoreModule/Config.h"

#include <chrono>

namespace Volt
{
	class MultiTimer
	{
	public:
		VTC_API MultiTimer(float resetTimeMilli = 500);

		VTC_API void Accumulate();
		VTC_API void Update();

		inline float GetMaxFrameTime() const { return m_currentMaxTime; }
		inline float GetAverageTime() const { return m_averageTime; }

		VTC_API float GetTime() const;
		VTC_API float GetDeltaTime() const;

	private:
		float m_accumulation = 0.f;
		uint32_t m_accumulationCount = 0;
		float m_accumulatedMaxTime = -FLT_MAX;

		float m_averageTime = 0.f;
		float m_currentMaxTime = -FLT_MAX;

		const float m_resetTime;

		std::chrono::steady_clock::time_point m_timeAtLastAccumulation;
		std::chrono::steady_clock::time_point m_timeAtLastUpdate;
	};
}
