#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Core.h>

#include <gtest/gtest.h>

#include <chrono>
#include <array>

namespace UnitTests
{
	struct WorkPayload
	{
		uint32_t counter;
	};

	using SPSCWorkQueue = WorkQueue<WorkPayload, QueueThreadingPolicy::SPSC>;
	using MPSCWorkQueue = WorkQueue<WorkPayload, QueueThreadingPolicy::MPSC>;
	using SPMCWorkQueue = WorkQueue<WorkPayload, QueueThreadingPolicy::SPMC>;
	using MPMCWorkQueue = WorkQueue<WorkPayload, QueueThreadingPolicy::MPMC>;

	TEST(SPSCWorkQueue, EmplacePop_ST)
	{
		constexpr uint32_t NumWorkItems = 100;

		// Does emplace and pop on a single thread.
		SPSCWorkQueue workQueue;
		workQueue.Allocate(NumWorkItems);

		// Emplace work items
		for (uint32_t i = 0; i < NumWorkItems; ++i)
		{
			workQueue.Emplace(i);
		}

		// Pop work items
		WorkPayload poppedPayload;
		uint32_t numWorkItems = 0;

		while (workQueue.Pop(poppedPayload))
		{
			numWorkItems++;
		}

		ASSERT_EQ(NumWorkItems, numWorkItems);
	}

	TEST(SPSCWorkQueue, EmplacePop_MT)
	{
		constexpr uint32_t NumWorkItems = 100;

		// Does emplace and pop on a two threads.
		SPSCWorkQueue workQueue;
		workQueue.Allocate(NumWorkItems);

		std::jthread pushThread([&workQueue]() 
		{
			// Emplace work items
			for (uint32_t i = 0; i < NumWorkItems; ++i)
			{
				workQueue.Emplace(i);
			}
		});

		uint32_t numPoppedItems = 0;
		bool failed = false;

		auto timePoint = std::chrono::system_clock::now();

		std::jthread popThread([&workQueue, &numPoppedItems, &timePoint, &failed]()
		{
			while (numPoppedItems < NumWorkItems && !failed)
			{
				WorkPayload payload;
				if (workQueue.Pop(payload))
				{
					numPoppedItems++;
				}

				failed = std::chrono::duration_cast<std::chrono::seconds>(timePoint - std::chrono::system_clock::now()).count() > 5.f;
			}
		});

		pushThread.join();
		popThread.join();

		ASSERT_FALSE(failed);
		ASSERT_EQ(numPoppedItems, NumWorkItems);
	}

	TEST(MPSCWorkQueue, EmplacePop_ST)
	{
		constexpr uint32_t NumWorkItems = 100;

		// Does emplace and pop on a single thread.
		MPSCWorkQueue workQueue;
		workQueue.Allocate(NumWorkItems);

		// Emplace work items
		for (uint32_t i = 0; i < NumWorkItems; ++i)
		{
			workQueue.Emplace(i);
		}

		// Pop work items
		WorkPayload poppedPayload;
		uint32_t numWorkItems = 0;

		while (workQueue.Pop(poppedPayload))
		{
			numWorkItems++;
		}

		ASSERT_EQ(NumWorkItems, numWorkItems);
	}

	TEST(MPSCWorkQueue, EmplacePop_MT)
	{
		constexpr uint32_t NumWorkItems = 1000;
		constexpr uint32_t NumWorkers = 4;

		MPSCWorkQueue workQueue;
		workQueue.Allocate(NumWorkItems);

		auto emplaceWorkerFunc = [&workQueue]()
		{
			for (uint32_t i = 0; i < (NumWorkItems / NumWorkers); ++i)
			{
				workQueue.Emplace(i);
			}
		};

		// Run worker threads
		Vector<Ref<std::jthread>> pushingThreads;
		for (uint32_t i = 0; i < NumWorkers; ++i)
		{
			pushingThreads.emplace_back(CreateRef<std::jthread>(emplaceWorkerFunc));
		}

		uint32_t numPoppedItems = 0;
		bool failed = false;
		auto timePoint = std::chrono::system_clock::now();

		std::jthread popThread([&workQueue, &numPoppedItems, &timePoint, &failed]()
		{
			while (numPoppedItems < NumWorkItems && !failed)
			{
				WorkPayload payload;
				if (workQueue.Pop(payload))
				{
					numPoppedItems++;
				}

				failed = std::chrono::duration_cast<std::chrono::seconds>(timePoint - std::chrono::system_clock::now()).count() > 5.f;
			}
		});

		for (auto thread : pushingThreads)
		{
			thread->join();
		}

		popThread.join();

		ASSERT_FALSE(failed);
		ASSERT_EQ(numPoppedItems, NumWorkItems);
	}

	TEST(SPMCWorkQueue, EmplacePop_ST)
	{
		constexpr uint32_t NumWorkItems = 100;

		// Does emplace and pop on a single thread.
		SPMCWorkQueue workQueue;
		workQueue.Allocate(NumWorkItems);

		// Emplace work items
		for (uint32_t i = 0; i < NumWorkItems; ++i)
		{
			workQueue.Emplace(i);
		}

		// Pop work items
		WorkPayload poppedPayload;
		uint32_t numWorkItems = 0;

		while (workQueue.Pop(poppedPayload))
		{
			numWorkItems++;
		}

		ASSERT_EQ(NumWorkItems, numWorkItems);
	}

	TEST(SPMCWorkQueue, EmplacePop_MT)
	{
		constexpr uint32_t NumWorkItems = 10000;
		constexpr uint32_t NumWorkers = 4;

		SPMCWorkQueue workQueue;
		workQueue.Allocate(NumWorkItems);

		std::jthread pushThread([&workQueue]()
		{
			for (uint32_t i = 0; i < NumWorkItems; ++i)
			{
				workQueue.Emplace(i);
			}
		});

		// Run worker threads
		std::atomic<uint32_t> numPoppedItems = 0;
		volatile bool failed = false;
		auto timePoint = std::chrono::system_clock::now();

		auto popWorkerFunc = [&workQueue, &numPoppedItems, &failed, &timePoint]() 
		{
			bool localFail = false;
			while (numPoppedItems < NumWorkItems && !localFail)
			{
				WorkPayload payload;
				if (workQueue.Pop(payload))
				{
					numPoppedItems++;
				}

				localFail = std::chrono::duration_cast<std::chrono::seconds>(timePoint - std::chrono::system_clock::now()).count() > 5.f;
				if (localFail)
				{
					failed |= true;
				}
			}
		};

		Vector<Ref<std::jthread>> poppingThreads;
		for (uint32_t i = 0; i < NumWorkers; ++i)
		{
			poppingThreads.emplace_back(CreateRef<std::jthread>(popWorkerFunc));
		}

		for (auto thread : poppingThreads)
		{
			thread->join();
		}

		pushThread.join();

		ASSERT_FALSE(failed);
		ASSERT_EQ(numPoppedItems, NumWorkItems);
	}

	TEST(MPMCWorkQueue, EmplacePop_ST)
	{
		constexpr uint32_t NumWorkItems = 100;

		// Does emplace and pop on a single thread.
		MPMCWorkQueue workQueue;
		workQueue.Allocate(NumWorkItems);

		// Emplace work items
		for (uint32_t i = 0; i < NumWorkItems; ++i)
		{
			workQueue.Emplace(i);
		}

		// Pop work items
		WorkPayload poppedPayload;
		uint32_t numWorkItems = 0;

		while (workQueue.Pop(poppedPayload))
		{
			numWorkItems++;
		}

		ASSERT_EQ(NumWorkItems, numWorkItems);
	}

	TEST(MPMCWorkQueue, EmplacePop_MT)
	{
		constexpr uint32_t NumWorkItems = 10000;
		constexpr uint32_t NumWorkers = 4;

		MPMCWorkQueue workQueue;
		workQueue.Allocate(NumWorkItems);

		auto emplaceWorkerFunc = [&workQueue]()
		{
			for (uint32_t i = 0; i < (NumWorkItems / NumWorkers); ++i)
			{
				workQueue.Emplace(i);
			}
		};

		// Run worker threads
		Vector<Ref<std::jthread>> pushingThreads;
		for (uint32_t i = 0; i < NumWorkers; ++i)
		{
			pushingThreads.emplace_back(CreateRef<std::jthread>(emplaceWorkerFunc));
		}

		// Run worker threads
		std::atomic<uint32_t> numPoppedItems = 0;
		volatile bool failed = false;
		auto timePoint = std::chrono::system_clock::now();

		auto popWorkerFunc = [&workQueue, &numPoppedItems, &failed, &timePoint]()
		{
			bool localFail = false;
			while (numPoppedItems < NumWorkItems && !localFail)
			{
				WorkPayload payload;
				if (workQueue.Pop(payload))
				{
					numPoppedItems++;
				}

				localFail = std::chrono::duration_cast<std::chrono::seconds>(timePoint - std::chrono::system_clock::now()).count() > 5.f;
				if (localFail)
				{
					failed |= true;
				}
			}
		};

		Vector<Ref<std::jthread>> poppingThreads;
		for (uint32_t i = 0; i < NumWorkers; ++i)
		{
			poppingThreads.emplace_back(CreateRef<std::jthread>(popWorkerFunc));
		}

		for (auto thread : poppingThreads)
		{
			thread->join();
		}

		for (auto thread : pushingThreads)
		{
			thread->join();
		}

		ASSERT_FALSE(failed);
		ASSERT_EQ(numPoppedItems, NumWorkItems);
	}
}
