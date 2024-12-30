#include "pipch.h"

#include "PhysicsInterface/PhysicsSubStepper.h"

namespace Volt
{
	PhysicsSubStepper::PhysicsSubStepper(float subStepSize)
		: m_subStepSize(subStepSize)
	{
	}

	uint32_t PhysicsSubStepper::Advance(float timeStep)
	{
		SubStepStrategy(timeStep);
		return m_numSubSteps;
	}

	void PhysicsSubStepper::SubStepStrategy(float timeStep)
	{
		if (m_accumulator > m_subStepSize)
		{
			m_accumulator = 0.f;
		}

		m_accumulator += timeStep;
		if (m_accumulator < m_subStepSize)
		{
			m_numSubSteps = 0;
			return;
		}

		m_numSubSteps = std::min(static_cast<uint32_t>(m_accumulator / m_subStepSize), m_maxSubSteps);
		m_accumulator -= static_cast<float>(m_numSubSteps) * m_subStepSize;
	}
}
