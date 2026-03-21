#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>
#include <CoreUtilities/Pointers/Ref.h>

#include <gtest/gtest.h>

namespace UnitTests
{
	TEST(PagedAtomicArenaAllocator, ReservePages_1)
	{
		PagedAtomicArenaAllocator<uint32_t, 1> allocator;
		allocator.ReservePages(1);

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 1);
	}

	TEST(PagedAtomicArenaAllocator, ReservePages_10)
	{
		PagedAtomicArenaAllocator<uint32_t, 1> allocator;
		allocator.ReservePages(10);

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 10);
	}

	TEST(PagedAtomicArenaAllocator, Allocate_1)
	{
		PagedAtomicArenaAllocator<uint32_t, 1> allocator;
		uint32_t* allocation = allocator.Allocate();

		VT_UNUSED(allocation);
	}

	TEST(PagedAtomicArenaAllocator, Allocate_FillPage)
	{
		constexpr uint32_t PageSize = 10;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		for (uint32_t i = 0; i < PageSize; ++i)
		{
			uint32_t* allocation = allocator.Allocate();
			ASSERT_TRUE(allocator.IsPointerWithinArena(allocation));
		}

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 1);
	}

	TEST(PagedAtomicArenaAllocator, Allocate_TwoPages)
	{
		constexpr uint32_t PageSize = 10;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		for (uint32_t i = 0; i < PageSize * 2; ++i)
		{
			uint32_t* allocation = allocator.Allocate();
			ASSERT_TRUE(allocator.IsPointerWithinArena(allocation));
		}

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 2);
	}

	TEST(PagedAtomicArenaAllocator, Allocate_Multithreaded)
	{
		constexpr uint32_t NumWorkers = 100;
		constexpr uint32_t PageSize = 10;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		auto allocateWorkerFunc = [&allocator]()
		{
			uint32_t* allocation = allocator.Allocate();
			ASSERT_TRUE(allocator.IsPointerWithinArena(allocation));
		};

		// Run worker threads
		Vector<Ref<std::jthread>> pushingThreads;
		for (uint32_t i = 0; i < NumWorkers; ++i)
		{
			pushingThreads.emplace_back(CreateRef<std::jthread>(allocateWorkerFunc));
		}

		for (auto thread : pushingThreads)
		{
			thread->join();
		}

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 10);
	}

	TEST(PagedAtomicArenaAllocator, Iterate)
	{
		constexpr uint32_t PageSize = 10;

		PagedAtomicArenaAllocator<uint32_t, PageSize> allocator;

		Vector<uint32_t*> allocations;

		for (uint32_t i = 0; i < PageSize * 2; ++i)
		{
			uint32_t* allocation = allocator.Allocate();
			ASSERT_TRUE(allocator.IsPointerWithinArena(allocation));
		
			allocations.emplace_back(allocation);
		}

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 2);

		// Now we free a couple to get some space between the allocations.

		uint32_t numFreed = 0;
		for (uint32_t i = 0; i < PageSize * 2; ++i)
		{
			if (i % 3)
			{
				allocator.Free(allocations[i]);
				numFreed++;
			}
		}

		const uint32_t numExpectedIterations = PageSize * 2 - numFreed;

		uint32_t numIterations = 0;
		for (PagedAtomicArenaAllocator<uint32_t, PageSize>::Iterator it(allocator); it; ++it)
		{
			ASSERT_NE(*it, nullptr);
			numIterations++;
		}

		ASSERT_EQ(numIterations, numExpectedIterations);
	}
}
