#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/PagedArenaAllocator.h>
#include <CoreUtilities/Core.h>

#include <gtest/gtest.h>

namespace UnitTests
{
	TEST(PagedArenaAllocator, ReservePages_1)
	{
		PagedArenaAllocator<uint32_t, 1> allocator;
		allocator.ReservePages(1);

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 1);
	}

	TEST(PagedArenaAllocator, ReservePages_10)
	{
		PagedArenaAllocator<uint32_t, 1> allocator;
		allocator.ReservePages(10);

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 10);
	}

	TEST(PagedArenaAllocator, Allocate_1)
	{
		PagedArenaAllocator<uint32_t, 1> allocator;
		uint32_t* allocation = allocator.Allocate();

		VT_UNUSED(allocation);
	}

	TEST(PagedArenaAllocator, Allocate_FillPage)
	{
		constexpr uint32_t PageSize = 10;

		PagedArenaAllocator<uint32_t, PageSize> allocator;

		for (uint32_t i = 0; i < PageSize; ++i)
		{
			uint32_t* allocation = allocator.Allocate();
			ASSERT_TRUE(allocator.IsPointerWithinArena(allocation));
		}

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 1);
	}

	TEST(PagedArenaAllocator, Allocate_TwoPages)
	{
		constexpr uint32_t PageSize = 10;

		PagedArenaAllocator<uint32_t, PageSize> allocator;

		for (uint32_t i = 0; i < PageSize * 2; ++i)
		{
			uint32_t* allocation = allocator.Allocate();
			ASSERT_TRUE(allocator.IsPointerWithinArena(allocation));
		}

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 2);
	}

	TEST(PagedArenaAllocator, Allocate_Multithreaded)
	{
		constexpr uint32_t NumWorkers = 100;
		constexpr uint32_t PageSize = 10;

		PagedArenaAllocator<uint32_t, PageSize> allocator;

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
}
