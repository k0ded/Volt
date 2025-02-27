#pragma once

#include "PhysicsInterface/Config.h"

namespace Volt
{
	class VTPI_API PhysicsSubStepper
	{
	public:
		PhysicsSubStepper(float subStepSize);

		uint32_t Advance(float timeStep);

	private:
		void SubStepStrategy(float timeStep);

		float m_accumulator = 0.f;
		uint32_t m_numSubSteps = 0;

		const float m_subStepSize;
		const uint32_t m_maxSubSteps = 8;
	};
}
