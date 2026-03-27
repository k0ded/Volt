#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>
#include <CoreUtilities/Pointers/Ref.h>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <set>
#include <mutex>
#include <barrier>
#include <latch>

namespace UnitTests
{
	// ===== Single-threaded baseline tests =====
	TEST(PagedAtomicArenaAllocator, ReservePages)
	{
		constexpr uint32_t NumPages = 10;

		PagedAtomicArenaAllocator<uint32_t, 64> allocator;
		allocator.ReservePages(NumPages);

		const uint64_t numAllocatedPages = allocator.GetNumAllocatedPages();
		ASSERT_EQ(numAllocatedPages, NumPages);
	}

	TEST(PagedAtomicArenaAllocator, Allocate_Single)
	{
		PagedAtomicArenaAllocator<uint32_t, 64> allocator;
		uint32_t* allocation = allocator.Allocate(42u);

		ASSERT_NE(allocation, nullptr);
		ASSERT_EQ(*allocation, 42u);

		allocator.Free(allocation);
	}

	TEST(PagedAtomicArenaAllocator, Allocate_FillPage)
	{
		constexpr uint64_t PageSize = 16;
		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		Vector<uint32_t*> allocations;
		for (uint64_t i = 0; i < PageSize; ++i)
		{
			uint32_t* ptr = allocator.Allocate(static_cast<uint32_t>(i));
			ASSERT_NE(ptr, nullptr);
			ASSERT_EQ(*ptr, static_cast<uint32_t>(i));
			allocations.emplace_back(ptr);
		}

		// All pointers should be unique.
		for (uint64_t i = 0; i < PageSize; ++i)
		{
			for (uint64_t j = i + 1; j < PageSize; ++j)
			{
				ASSERT_NE(allocations[i], allocations[j]);
			}
		}

		for (auto* ptr : allocations)
		{
			allocator.Free(ptr);
		}
	}

	TEST(PagedAtomicArenaAllocator, Allocate_SpansMultiplePages)
	{
		constexpr uint64_t PageSize = 8;
		constexpr uint64_t NumAllocations = PageSize * 3;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		Vector<uint32_t*> allocations;
		for (uint64_t i = 0; i < NumAllocations; ++i)
		{
			uint32_t* ptr = allocator.Allocate(static_cast<uint32_t>(i));
			ASSERT_NE(ptr, nullptr);
			allocations.emplace_back(ptr);
		}

		// All pointers unique.
		for (uint64_t i = 0; i < NumAllocations; ++i)
		{
			for (uint64_t j = i + 1; j < NumAllocations; ++j)
			{
				ASSERT_NE(allocations[i], allocations[j]);
			}
		}

		for (auto* ptr : allocations)
		{
			allocator.Free(ptr);
		}
	}

	TEST(PagedAtomicArenaAllocator, AllocateAndFree_Reuse)
	{
		constexpr uint64_t PageSize = 4;
		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		// Fill the page.
		Vector<uint32_t*> allocations;
		for (uint64_t i = 0; i < PageSize; ++i)
		{
			allocations.emplace_back(allocator.Allocate(0u));
			ASSERT_NE(allocations.back(), nullptr);
		}

		const uint64_t numPages = allocator.GetNumAllocatedPages();

		// Free all.
		for (auto* ptr : allocations)
		{
			allocator.Free(ptr);
		}

		// Allocate again; should succeed without needing new pages.
		for (uint64_t i = 0; i < PageSize; ++i)
		{
			uint32_t* ptr = allocator.Allocate(0u);
			ASSERT_NE(ptr, nullptr);
			allocator.Free(ptr);
		}

		const uint64_t numPagesAfterReuse = allocator.GetNumAllocatedPages();
		ASSERT_EQ(numPages, numPagesAfterReuse);
	}

	TEST(PagedAtomicArenaAllocator, MoveConstructor)
	{
		PagedAtomicArenaAllocator<uint32_t, 16> allocator;
		uint32_t* ptr = allocator.Allocate(99u);
		ASSERT_NE(ptr, nullptr);

		PagedAtomicArenaAllocator<uint32_t, 16> moved(std::move(allocator));

		// The moved-to allocator should own the allocation.
		ASSERT_EQ(*ptr, 99u);
		moved.Free(ptr);
	}

	TEST(PagedAtomicArenaAllocator, MoveAssignment)
	{
		PagedAtomicArenaAllocator<uint32_t, 16> allocator;
		uint32_t* ptr = allocator.Allocate(77u);
		ASSERT_NE(ptr, nullptr);

		PagedAtomicArenaAllocator<uint32_t, 16> other;
		other = std::move(allocator);

		ASSERT_EQ(*ptr, 77u);
		other.Free(ptr);
	}

	// ===== Multi-threaded tests =====

	TEST(PagedAtomicArenaAllocator, MT_ConcurrentAllocate)
	{
		constexpr uint32_t NumThreads = 8;
		constexpr uint32_t AllocationsPerThread = 100;
		constexpr uint64_t PageSize = 64;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		std::mutex mutex;
		Vector<uint32_t*> allAllocations;

		std::latch startLatch(NumThreads);

		auto workerFunc = [&](uint32_t threadId)
		{
			Vector<uint32_t*> localAllocations;
			localAllocations.reserve(AllocationsPerThread);

			// Wait for all threads to be ready for maximum contention.
			startLatch.count_down();
			startLatch.wait();

			for (uint32_t i = 0; i < AllocationsPerThread; ++i)
			{
				const uint32_t value = threadId * AllocationsPerThread + i;
				uint32_t* ptr = allocator.Allocate(value);
				ASSERT_NE(ptr, nullptr);
				ASSERT_EQ(*ptr, value);
				localAllocations.emplace_back(ptr);
			}

			std::lock_guard lock(mutex);
			for (auto* ptr : localAllocations)
			{
				allAllocations.emplace_back(ptr);
			}
		};

		Vector<Ref<std::jthread>> threads;
		for (uint32_t t = 0; t < NumThreads; ++t)
		{
			threads.emplace_back(CreateRef<std::jthread>(workerFunc, t));
		}

		for (auto& thread : threads)
		{
			thread->join();
		}

		const uint32_t totalAllocations = NumThreads * AllocationsPerThread;
		ASSERT_EQ(static_cast<uint32_t>(allAllocations.size()), totalAllocations);

		// Verify all pointers are unique (no two threads got the same slot).
		std::set<uint32_t*> uniquePtrs(allAllocations.begin(), allAllocations.end());
		ASSERT_EQ(uniquePtrs.size(), static_cast<size_t>(totalAllocations));

		// Clean up.
		for (auto* ptr : allAllocations)
		{
			allocator.Free(ptr);
		}
	}

	TEST(PagedAtomicArenaAllocator, MT_ConcurrentAllocateAndFree)
	{
		constexpr uint32_t NumThreads = 8;
		constexpr uint32_t IterationsPerThread = 200;
		constexpr uint64_t PageSize = 32;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		std::latch startLatch(NumThreads);

		auto workerFunc = [&](uint32_t threadId)
		{
			startLatch.count_down();
			startLatch.wait();

			for (uint32_t i = 0; i < IterationsPerThread; ++i)
			{
				const uint32_t value = threadId * IterationsPerThread + i;
				uint32_t* ptr = allocator.Allocate(value);
				ASSERT_NE(ptr, nullptr);
				ASSERT_EQ(*ptr, value);
				allocator.Free(ptr);
			}
		};

		Vector<Ref<std::jthread>> threads;
		for (uint32_t t = 0; t < NumThreads; ++t)
		{
			threads.emplace_back(CreateRef<std::jthread>(workerFunc, t));
		}

		for (auto& thread : threads)
		{
			thread->join();
		}
	}

	TEST(PagedAtomicArenaAllocator, MT_HighContention_SmallPage)
	{
		// Force very high contention by using a tiny page size with many threads.
		constexpr uint32_t NumThreads = 16;
		constexpr uint32_t AllocationsPerThread = 50;
		constexpr uint64_t PageSize = 4;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		std::mutex mutex;
		Vector<uint32_t*> allAllocations;

		std::latch startLatch(NumThreads);

		auto workerFunc = [&](uint32_t threadId)
		{
			Vector<uint32_t*> localAllocations;
			localAllocations.reserve(AllocationsPerThread);

			startLatch.count_down();
			startLatch.wait();

			for (uint32_t i = 0; i < AllocationsPerThread; ++i)
			{
				uint32_t* ptr = allocator.Allocate(threadId);
				ASSERT_NE(ptr, nullptr);
				localAllocations.emplace_back(ptr);
			}

			std::lock_guard lock(mutex);
			for (auto* ptr : localAllocations)
			{
				allAllocations.emplace_back(ptr);
			}
		};

		Vector<Ref<std::jthread>> threads;
		for (uint32_t t = 0; t < NumThreads; ++t)
		{
			threads.emplace_back(CreateRef<std::jthread>(workerFunc, t));
		}

		for (auto& thread : threads)
		{
			thread->join();
		}

		const uint32_t totalAllocations = NumThreads * AllocationsPerThread;
		ASSERT_EQ(static_cast<uint32_t>(allAllocations.size()), totalAllocations);

		// Uniqueness check.
		std::set<uint32_t*> uniquePtrs(allAllocations.begin(), allAllocations.end());
		ASSERT_EQ(uniquePtrs.size(), static_cast<size_t>(totalAllocations));

		for (auto* ptr : allAllocations)
		{
			allocator.Free(ptr);
		}
	}

	TEST(PagedAtomicArenaAllocator, MT_AllocateFreeCycles)
	{
		// Threads repeatedly allocate a batch, then free the batch,
		// exercising slot reuse under contention.
		constexpr uint32_t NumThreads = 8;
		constexpr uint32_t CyclesPerThread = 20;
		constexpr uint32_t BatchSize = 10;
		constexpr uint64_t PageSize = 32;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		std::latch startLatch(NumThreads);

		auto workerFunc = [&]()
		{
			startLatch.count_down();
			startLatch.wait();

			for (uint32_t cycle = 0; cycle < CyclesPerThread; ++cycle)
			{
				Vector<uint32_t*> batch;
				batch.reserve(BatchSize);

				for (uint32_t i = 0; i < BatchSize; ++i)
				{
					uint32_t* ptr = allocator.Allocate(i);
					ASSERT_NE(ptr, nullptr);
					batch.emplace_back(ptr);
				}

				for (auto* ptr : batch)
				{
					allocator.Free(ptr);
				}
			}
		};

		Vector<Ref<std::jthread>> threads;
		for (uint32_t t = 0; t < NumThreads; ++t)
		{
			threads.emplace_back(CreateRef<std::jthread>(workerFunc));
		}

		for (auto& thread : threads)
		{
			thread->join();
		}
	}

	TEST(PagedAtomicArenaAllocator, MT_ProducerConsumer)
	{
		// Half the threads allocate and push pointers, the other half pop and free.
		constexpr uint32_t NumProducers = 4;
		constexpr uint32_t NumConsumers = 4;
		constexpr uint32_t ItemsPerProducer = 100;
		constexpr uint64_t PageSize = 64;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		std::mutex queueMutex;
		Vector<uint32_t*> sharedQueue;
		std::atomic<uint32_t> producersDone{ 0 };

		std::latch startLatch(NumProducers + NumConsumers);

		auto producerFunc = [&](uint32_t producerId)
		{
			startLatch.count_down();
			startLatch.wait();

			for (uint32_t i = 0; i < ItemsPerProducer; ++i)
			{
				uint32_t* ptr = allocator.Allocate(producerId * ItemsPerProducer + i);
				ASSERT_NE(ptr, nullptr);

				std::lock_guard lock(queueMutex);
				sharedQueue.emplace_back(ptr);
			}

			producersDone.fetch_add(1, std::memory_order::relaxed);
		};

		std::atomic<uint32_t> totalFreed{ 0 };

		auto consumerFunc = [&]()
		{
			startLatch.count_down();
			startLatch.wait();

			while (true)
			{
				uint32_t* ptr = nullptr;
				{
					std::lock_guard lock(queueMutex);
					if (!sharedQueue.empty())
					{
						ptr = sharedQueue.back();
						sharedQueue.pop_back();
					}
				}

				if (ptr != nullptr)
				{
					allocator.Free(ptr);
					totalFreed.fetch_add(1, std::memory_order::relaxed);
				}
				else if (producersDone.load(std::memory_order::relaxed) == NumProducers)
				{
					// Check once more with the lock held to avoid race.
					std::lock_guard lock(queueMutex);
					if (sharedQueue.empty())
					{
						break;
					}
				}
				else
				{
					std::this_thread::yield();
				}
			}
		};

		Vector<Ref<std::jthread>> threads;
		for (uint32_t p = 0; p < NumProducers; ++p)
		{
			threads.emplace_back(CreateRef<std::jthread>(producerFunc, p));
		}
		for (uint32_t c = 0; c < NumConsumers; ++c)
		{
			threads.emplace_back(CreateRef<std::jthread>(consumerFunc));
		}

		for (auto& thread : threads)
		{
			thread->join();
		}

		ASSERT_EQ(totalFreed.load(), NumProducers * ItemsPerProducer);
	}

	TEST(PagedAtomicArenaAllocator, MT_ValueIntegrity)
	{
		// Verify that written values are not corrupted by concurrent allocations.
		constexpr uint32_t NumThreads = 8;
		constexpr uint32_t AllocationsPerThread = 100;
		constexpr uint64_t PageSize = 64;

		PagedAtomicArenaAllocator<uint64_t, PageSize> allocator;

		std::mutex mutex;
		Vector<std::pair<uint64_t*, uint64_t>> allAllocations;

		std::latch startLatch(NumThreads);

		auto workerFunc = [&](uint32_t threadId)
		{
			Vector<std::pair<uint64_t*, uint64_t>> localAllocations;

			startLatch.count_down();
			startLatch.wait();

			for (uint32_t i = 0; i < AllocationsPerThread; ++i)
			{
				const uint64_t value = (static_cast<uint64_t>(threadId) << 32) | i;
				uint64_t* ptr = allocator.Allocate(value);
				ASSERT_NE(ptr, nullptr);
				localAllocations.emplace_back(ptr, value);
			}

			std::lock_guard lock(mutex);
			for (const auto& pair : localAllocations)
			{
				allAllocations.emplace_back(pair);
			}
		};

		Vector<Ref<std::jthread>> threads;
		for (uint32_t t = 0; t < NumThreads; ++t)
		{
			threads.emplace_back(CreateRef<std::jthread>(workerFunc, t));
		}

		for (auto& thread : threads)
		{
			thread->join();
		}

		// Verify every allocation still holds the value that was written.
		for (const auto& [ptr, expectedValue] : allAllocations)
		{
			ASSERT_EQ(*ptr, expectedValue) << "Value corruption detected!";
		}

		for (const auto& [ptr, _] : allAllocations)
		{
			allocator.Free(ptr);
		}
	}

	struct LifetimeTracker
	{
		inline static std::atomic<int32_t> s_aliveCount{ 0 };

		LifetimeTracker() { s_aliveCount.fetch_add(1, std::memory_order::relaxed); }
		~LifetimeTracker() { s_aliveCount.fetch_sub(1, std::memory_order::relaxed); }

		LifetimeTracker(const LifetimeTracker&) { s_aliveCount.fetch_add(1, std::memory_order::relaxed); }
		LifetimeTracker& operator=(const LifetimeTracker&) = default;

		LifetimeTracker(LifetimeTracker&&) noexcept { s_aliveCount.fetch_add(1, std::memory_order::relaxed); }
		LifetimeTracker& operator=(LifetimeTracker&&) noexcept = default;
	};

	TEST(PagedAtomicArenaAllocator, MT_ConstructorDestructorCalled)
	{
		// Verify constructors and destructors are called exactly once per allocation.
		constexpr uint32_t NumThreads = 8;
		constexpr uint32_t AllocationsPerThread = 50;
		constexpr uint64_t PageSize = 64;

		LifetimeTracker::s_aliveCount.store(0, std::memory_order::relaxed);

		PagedAtomicArenaAllocator<LifetimeTracker, PageSize> allocator;

		std::mutex mutex;
		Vector<LifetimeTracker*> allAllocations;

		std::latch startLatch(NumThreads);

		auto workerFunc = [&]()
		{
			Vector<LifetimeTracker*> localAllocations;

			startLatch.count_down();
			startLatch.wait();

			for (uint32_t i = 0; i < AllocationsPerThread; ++i)
			{
				LifetimeTracker* ptr = allocator.Allocate();
				ASSERT_NE(ptr, nullptr);
				localAllocations.emplace_back(ptr);
			}

			std::lock_guard lock(mutex);
			for (auto* ptr : localAllocations)
			{
				allAllocations.emplace_back(ptr);
			}
		};

		{
			Vector<Ref<std::jthread>> threads;
			for (uint32_t t = 0; t < NumThreads; ++t)
			{
				threads.emplace_back(CreateRef<std::jthread>(workerFunc));
			}

			for (auto& thread : threads)
			{
				thread->join();
			}
		}

		const uint32_t totalAllocations = NumThreads * AllocationsPerThread;
		ASSERT_EQ(LifetimeTracker::s_aliveCount.load(), static_cast<int32_t>(totalAllocations));

		for (auto* ptr : allAllocations)
		{
			allocator.Free(ptr);
		}

		ASSERT_EQ(LifetimeTracker::s_aliveCount.load(), 0);
	}

	TEST(PagedAtomicArenaAllocator, MT_StressTest)
	{
		// Large-scale stress test with many threads and allocations.
		constexpr uint32_t NumThreads = 16;
		constexpr uint32_t AllocationsPerThread = 500;
		constexpr uint64_t PageSize = 128;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		std::atomic<uint32_t> successCount{ 0 };

		std::latch startLatch(NumThreads);

		auto workerFunc = [&](uint32_t threadId)
		{
			startLatch.count_down();
			startLatch.wait();

			Vector<uint32_t*> localAllocations;
			localAllocations.reserve(AllocationsPerThread);

			for (uint32_t i = 0; i < AllocationsPerThread; ++i)
			{
				uint32_t* ptr = allocator.Allocate(threadId);
				ASSERT_NE(ptr, nullptr);
				localAllocations.emplace_back(ptr);
			}

			// Free half of them.
			for (uint32_t i = 0; i < AllocationsPerThread / 2; ++i)
			{
				allocator.Free(localAllocations[i]);
			}

			// Reallocate those slots.
			for (uint32_t i = 0; i < AllocationsPerThread / 2; ++i)
			{
				uint32_t* ptr = allocator.Allocate(threadId);
				ASSERT_NE(ptr, nullptr);
				localAllocations[i] = ptr;
			}

			// Free everything.
			for (auto* ptr : localAllocations)
			{
				allocator.Free(ptr);
			}

			successCount.fetch_add(1, std::memory_order::relaxed);
		};

		Vector<Ref<std::jthread>> threads;
		for (uint32_t t = 0; t < NumThreads; ++t)
		{
			threads.emplace_back(CreateRef<std::jthread>(workerFunc, t));
		}

		for (auto& thread : threads)
		{
			thread->join();
		}

		ASSERT_EQ(successCount.load(), NumThreads);
	}
}
