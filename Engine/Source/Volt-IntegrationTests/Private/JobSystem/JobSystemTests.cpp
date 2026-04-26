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
	}
}
