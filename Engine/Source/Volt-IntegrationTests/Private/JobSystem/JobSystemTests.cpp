#include "ApplicationFixture.h"

#include <CoreUtilities/Containers/Array.h>

using namespace Volt;

namespace IntegrationTests
{
	class JobSystemFixture : public ApplicationFixture
	{ };

	void ThreadFunc()
	{
		while (true)
		{
			JobCounterRef counter = JobSystem::CreateCounter();
		
			JobSystem::DestroyCounter(counter);
		}
	}

	TEST_F(JobSystemFixture, CounterAllocation)
	{
		Array<std::thread, 16> threads;

		for (uint32_t i = 0; i < 16; ++i)
		{
			threads[i] = std::thread(&ThreadFunc);
		}

		threads[0].join();
	}
}
