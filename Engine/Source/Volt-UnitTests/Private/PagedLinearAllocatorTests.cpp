#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/PagedLinearAllocator.h>
#include <CoreUtilities/Core.h>

#include <gtest/gtest.h>

namespace UnitTests
{
	// 1024 + 24 to account for header size.
	static constexpr uint64_t PageSize = 1048;

	TEST(PagedLinearAllocator, ReservePages_1)
	{
		PagedLinearAllocator<PageSize> allocator;
		allocator.ReservePages(1);

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 1);
	}

	TEST(PagedLinearAllocator, ReservePages_10)
	{
		PagedLinearAllocator<PageSize> allocator;
		allocator.ReservePages(10);

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 10);
	}

	TEST(PagedLinearAllocator, Allocate_1)
	{
		PagedLinearAllocator<PageSize> allocator;
		void* allocation = allocator.Allocate(512);
		VT_UNUSED(allocation);
	}

	TEST(PagedLinearAllocator, Allocate_FillPage)
	{
		PagedLinearAllocator<PageSize> allocator;
		void* allocation0 = allocator.Allocate(512);
		void* allocation1 = allocator.Allocate(512);
		VT_UNUSED(allocation0);
		VT_UNUSED(allocation1);

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 1);
	}

	TEST(PagedLinearAllocator, Allocate_TwoPages)
	{
		PagedLinearAllocator<PageSize> allocator;
		void* allocation0 = allocator.Allocate(512);
		void* allocation1 = allocator.Allocate(512);
		void* allocation2 = allocator.Allocate(512);
		VT_UNUSED(allocation0);
		VT_UNUSED(allocation1);
		VT_UNUSED(allocation2);

		ASSERT_EQ(allocator.GetNumAllocatedPages(), 2);
	}

	TEST(PagedLinearAllocator, Allocate_Multithreaded)
	{
		constexpr uint32_t NumWorkers = 100;

		// 1048 to account for header size.
		PagedLinearAllocator<PageSize> allocator;

		auto allocateWorkerFunc = [&allocator]()
		{
			void* allocation = allocator.Allocate(512);
			VT_UNUSED(allocation);
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

		// 512 / 1024 * 100 = 50.
		ASSERT_EQ(allocator.GetNumAllocatedPages(), 50);
	}
}
