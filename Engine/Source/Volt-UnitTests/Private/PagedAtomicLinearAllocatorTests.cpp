#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/PagedAtomicLinearAllocator.h>
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

	TEST(PagedAtomicLinearAllocator, DefaultConstruct_NoPages)
	{
		PagedAtomicLinearAllocator<1024> allocator;
		ASSERT_EQ(allocator.GetNumAllocatedPages(), 0u);
	}

	TEST(PagedAtomicLinearAllocator, ReservePages)
	{
		constexpr uint32_t NumPages = 10;

		PagedAtomicLinearAllocator<1024> allocator;
		allocator.ReservePages(NumPages);

		ASSERT_EQ(allocator.GetNumAllocatedPages(), NumPages);
	}

	TEST(PagedAtomicLinearAllocator, ReservePages_AlreadySufficient)
	{
		PagedAtomicLinearAllocator<1024> allocator;
		allocator.ReservePages(5);
		ASSERT_EQ(allocator.GetNumAllocatedPages(), 5u);

		// Requesting fewer pages should be a no-op.
		allocator.ReservePages(3);
		ASSERT_EQ(allocator.GetNumAllocatedPages(), 5u);
	}

	TEST(PagedAtomicLinearAllocator, Allocate_Single)
	{
		PagedAtomicLinearAllocator<1024> allocator;
		void* ptr = allocator.Allocate(16);

		ASSERT_NE(ptr, nullptr);
		ASSERT_EQ(allocator.GetNumAllocatedPages(), 1u);
	}

	TEST(PagedAtomicLinearAllocator, Allocate_MultipleWithinPage)
	{
		constexpr uint64_t PageSize = 256;
		constexpr uint64_t AllocationSize = 32;
		constexpr uint64_t NumAllocations = PageSize / AllocationSize;

		PagedAtomicLinearAllocator<PageSize> allocator;

		std::set<void*> ptrs;
		for (uint64_t i = 0; i < NumAllocations; ++i)
		{
			void* ptr = allocator.Allocate(AllocationSize);
			ASSERT_NE(ptr, nullptr);
			ptrs.insert(ptr);
		}

		// All pointers should be unique.
		ASSERT_EQ(ptrs.size(), static_cast<size_t>(NumAllocations));
	}

	TEST(PagedAtomicLinearAllocator, Allocate_SpansMultiplePages)
	{
		constexpr uint64_t PageSize = 64;
		constexpr uint64_t AllocationSize = 32;
		// Allocate enough to force at least 3 pages.
		constexpr uint64_t NumAllocations = (PageSize / AllocationSize) * 3;

		PagedAtomicLinearAllocator<PageSize> allocator;

		std::set<void*> ptrs;
		for (uint64_t i = 0; i < NumAllocations; ++i)
		{
			void* ptr = allocator.Allocate(AllocationSize);
			ASSERT_NE(ptr, nullptr);
			ptrs.insert(ptr);
		}

		ASSERT_EQ(ptrs.size(), static_cast<size_t>(NumAllocations));
		ASSERT_GE(allocator.GetNumAllocatedPages(), 3u);
	}

	TEST(PagedAtomicLinearAllocator, Allocate_LargerThanPageSize)
	{
		constexpr uint64_t PageSize = 64;

		PagedAtomicLinearAllocator<PageSize> allocator;

		// Allocate more than PageSize bytes; should still succeed by
		// allocating an oversized page.
		void* ptr = allocator.Allocate(PageSize * 2);
		ASSERT_NE(ptr, nullptr);
	}

	TEST(PagedAtomicLinearAllocator, Allocate_WritableMemory)
	{
		PagedAtomicLinearAllocator<1024> allocator;
		void* ptr = allocator.Allocate(sizeof(uint64_t));

		ASSERT_NE(ptr, nullptr);

		// Write and read back to verify the memory is usable.
		uint64_t* typed = static_cast<uint64_t*>(ptr);
		*typed = 0xDEADBEEFCAFEBABE;
		ASSERT_EQ(*typed, 0xDEADBEEFCAFEBABE);
	}

	TEST(PagedAtomicLinearAllocator, Reset)
	{
		constexpr uint64_t PageSize = 128;
		PagedAtomicLinearAllocator<PageSize> allocator;

		// Fill up a page.
		for (uint64_t i = 0; i < PageSize / 16; ++i)
		{
			allocator.Allocate(16);
		}

		const uint32_t pagesBeforeReset = allocator.GetNumAllocatedPages();

		allocator.Reset();

		// Pages should still be allocated (not freed).
		ASSERT_EQ(allocator.GetNumAllocatedPages(), pagesBeforeReset);

		// Should be able to allocate again from the beginning.
		void* ptr = allocator.Allocate(16);
		ASSERT_NE(ptr, nullptr);
	}

	TEST(PagedAtomicLinearAllocator, Reset_DoesNotAddPages)
	{
		constexpr uint64_t PageSize = 64;
		PagedAtomicLinearAllocator<PageSize> allocator;

		for (uint64_t i = 0; i < 4; ++i)
		{
			allocator.Allocate(PageSize);
		}

		const uint32_t pagesAfterFirstPass = allocator.GetNumAllocatedPages();

		allocator.Reset();

		// Allocate the same amount again.
		for (uint64_t i = 0; i < 4; ++i)
		{
			allocator.Allocate(PageSize);
		}

		// Should reuse existing pages, not allocate new ones.
		ASSERT_EQ(allocator.GetNumAllocatedPages(), pagesAfterFirstPass);
	}

	TEST(PagedAtomicLinearAllocator, MoveConstructor)
	{
		PagedAtomicLinearAllocator<256> allocator;
		void* ptr = allocator.Allocate(32);
		ASSERT_NE(ptr, nullptr);

		const uint32_t pagesBefore = allocator.GetNumAllocatedPages();

		PagedAtomicLinearAllocator<256> moved(std::move(allocator));

		ASSERT_EQ(moved.GetNumAllocatedPages(), pagesBefore);
		ASSERT_EQ(allocator.GetNumAllocatedPages(), 0u);
	}

	TEST(PagedAtomicLinearAllocator, MoveAssignment)
	{
		PagedAtomicLinearAllocator<256> allocator;
		allocator.Allocate(32);

		const uint32_t pagesBefore = allocator.GetNumAllocatedPages();

		PagedAtomicLinearAllocator<256> other;
		other = std::move(allocator);

		ASSERT_EQ(other.GetNumAllocatedPages(), pagesBefore);
		ASSERT_EQ(allocator.GetNumAllocatedPages(), 0u);
	}

	TEST(PagedAtomicLinearAllocator, MoveAssignment_SelfAssign)
	{
		PagedAtomicLinearAllocator<256> allocator;
		allocator.Allocate(32);
		const uint32_t pagesBefore = allocator.GetNumAllocatedPages();

		allocator = std::move(allocator);

		// Self-assignment should be a no-op.
		ASSERT_EQ(allocator.GetNumAllocatedPages(), pagesBefore);
	}

	TEST(PagedAtomicLinearAllocator, PageIterator_Empty)
	{
		PagedAtomicLinearAllocator<256> allocator;
		PagedAtomicLinearAllocator<256>::PageIterator it(allocator);

		ASSERT_FALSE(static_cast<bool>(it));
	}

	TEST(PagedAtomicLinearAllocator, PageIterator_TraversesAllPages)
	{
		constexpr uint64_t PageSize = 64;
		constexpr uint32_t NumPages = 4;

		PagedAtomicLinearAllocator<PageSize> allocator;

		// Force multiple pages by allocating full pages.
		for (uint32_t i = 0; i < NumPages; ++i)
		{
			allocator.Allocate(PageSize);
		}

		ASSERT_EQ(allocator.GetNumAllocatedPages(), NumPages);

		uint32_t iteratedPages = 0;
		for (PagedAtomicLinearAllocator<PageSize>::PageIterator it(allocator); it; ++it)
		{
			++iteratedPages;
		}

		ASSERT_EQ(iteratedPages, NumPages);
	}

	// ===== Multi-threaded tests =====

	TEST(PagedAtomicLinearAllocator, MT_ConcurrentAllocate)
	{
		constexpr uint32_t NumThreads = 8;
		constexpr uint32_t AllocationsPerThread = 200;
		constexpr uint64_t AllocationSize = 32;
		constexpr uint64_t PageSize = 1024;

		PagedAtomicLinearAllocator<PageSize> allocator;

		std::mutex mutex;
		Vector<void*> allAllocations;

		std::latch startLatch(NumThreads);

		auto workerFunc = [&]()
		{
			Vector<void*> localAllocations;
			localAllocations.reserve(AllocationsPerThread);

			startLatch.count_down();
			startLatch.wait();

			for (uint32_t i = 0; i < AllocationsPerThread; ++i)
			{
				void* ptr = allocator.Allocate(AllocationSize);
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
			threads.emplace_back(CreateRef<std::jthread>(workerFunc));
		}

		for (auto& thread : threads)
		{
			thread->join();
		}

		const uint32_t totalAllocations = NumThreads * AllocationsPerThread;
		ASSERT_EQ(static_cast<uint32_t>(allAllocations.size()), totalAllocations);

		// Verify all pointers are unique (no two threads got the same memory).
		std::set<void*> uniquePtrs(allAllocations.begin(), allAllocations.end());
		ASSERT_EQ(uniquePtrs.size(), static_cast<size_t>(totalAllocations));
	}

	TEST(PagedAtomicLinearAllocator, MT_HighContention_SmallPage)
	{
		// Force very high contention by using a tiny page size with many threads.
		constexpr uint32_t NumThreads = 16;
		constexpr uint32_t AllocationsPerThread = 100;
		constexpr uint64_t AllocationSize = 16;
		constexpr uint64_t PageSize = 64;

		PagedAtomicLinearAllocator<PageSize> allocator;

		std::mutex mutex;
		Vector<void*> allAllocations;

		std::latch startLatch(NumThreads);

		auto workerFunc = [&]()
		{
			Vector<void*> localAllocations;
			localAllocations.reserve(AllocationsPerThread);

			startLatch.count_down();
			startLatch.wait();

			for (uint32_t i = 0; i < AllocationsPerThread; ++i)
			{
				void* ptr = allocator.Allocate(AllocationSize);
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
			threads.emplace_back(CreateRef<std::jthread>(workerFunc));
		}

		for (auto& thread : threads)
		{
			thread->join();
		}

		const uint32_t totalAllocations = NumThreads * AllocationsPerThread;
		ASSERT_EQ(static_cast<uint32_t>(allAllocations.size()), totalAllocations);

		// Uniqueness check.
		std::set<void*> uniquePtrs(allAllocations.begin(), allAllocations.end());
		ASSERT_EQ(uniquePtrs.size(), static_cast<size_t>(totalAllocations));
	}

	TEST(PagedAtomicLinearAllocator, MT_ConcurrentAllocate_VerifyWrites)
	{
		// Each thread writes a unique pattern into its allocations and
		// verifies them after all threads complete.
		constexpr uint32_t NumThreads = 8;
		constexpr uint32_t AllocationsPerThread = 100;
		constexpr uint64_t PageSize = 2048;

		PagedAtomicLinearAllocator<PageSize> allocator;

		struct TaggedAllocation
		{
			uint32_t* ptr;
			uint32_t expectedValue;
		};

		std::mutex mutex;
		Vector<TaggedAllocation> allAllocations;

		std::latch startLatch(NumThreads);

		auto workerFunc = [&](uint32_t threadId)
		{
			Vector<TaggedAllocation> localAllocations;
			localAllocations.reserve(AllocationsPerThread);

			startLatch.count_down();
			startLatch.wait();

			for (uint32_t i = 0; i < AllocationsPerThread; ++i)
			{
				void* raw = allocator.Allocate(sizeof(uint32_t));
				ASSERT_NE(raw, nullptr);

				const uint32_t value = threadId * AllocationsPerThread + i;
				uint32_t* typed = static_cast<uint32_t*>(raw);
				*typed = value;

				localAllocations.emplace_back(TaggedAllocation{ typed, value });
			}

			std::lock_guard lock(mutex);
			for (auto& tagged : localAllocations)
			{
				allAllocations.emplace_back(tagged);
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

		// Verify every allocation still holds its expected value.
		for (const auto& tagged : allAllocations)
		{
			ASSERT_EQ(*tagged.ptr, tagged.expectedValue);
		}
	}

	TEST(PagedAtomicLinearAllocator, MT_ConcurrentAllocate_VaryingSizes)
	{
		// Threads allocate varying sizes to stress the bump-pointer logic
		// with different offsets racing simultaneously.
		constexpr uint32_t NumThreads = 8;
		constexpr uint32_t AllocationsPerThread = 150;
		constexpr uint64_t PageSize = 512;

		PagedAtomicLinearAllocator<PageSize> allocator;

		std::mutex mutex;
		Vector<void*> allAllocations;

		std::latch startLatch(NumThreads);

		auto workerFunc = [&](uint32_t threadId)
		{
			Vector<void*> localAllocations;
			localAllocations.reserve(AllocationsPerThread);

			startLatch.count_down();
			startLatch.wait();

			for (uint32_t i = 0; i < AllocationsPerThread; ++i)
			{
				// Vary the size: 8, 16, 24, 32, cycling through.
				const uint64_t size = ((i % 4) + 1) * 8;
				void* ptr = allocator.Allocate(size);
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

		std::set<void*> uniquePtrs(allAllocations.begin(), allAllocations.end());
		ASSERT_EQ(uniquePtrs.size(), static_cast<size_t>(totalAllocations));
	}

	TEST(PagedAtomicLinearAllocator, MT_ResetThenReallocate)
	{
		// Allocate from multiple threads, reset, then allocate again
		// to verify pages are correctly reused.
		constexpr uint32_t NumThreads = 8;
		constexpr uint32_t AllocationsPerThread = 50;
		constexpr uint64_t AllocationSize = 16;
		constexpr uint64_t PageSize = 256;

		PagedAtomicLinearAllocator<PageSize> allocator;

		auto allocatePass = [&]()
		{
			std::latch startLatch(NumThreads);

			auto workerFunc = [&]()
			{
				startLatch.count_down();
				startLatch.wait();

				for (uint32_t i = 0; i < AllocationsPerThread; ++i)
				{
					void* ptr = allocator.Allocate(AllocationSize);
					ASSERT_NE(ptr, nullptr);
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
		};

		// First pass.
		allocatePass();
		const uint32_t pagesAfterFirstPass = allocator.GetNumAllocatedPages();

		// Reset and do it again.
		allocator.Reset();
		allocatePass();

		// Should reuse pages, not allocate more.
		ASSERT_EQ(allocator.GetNumAllocatedPages(), pagesAfterFirstPass);
	}
}
