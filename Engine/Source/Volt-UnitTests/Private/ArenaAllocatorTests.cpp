#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/FixedSizeArenaAllocator.h>
#include <CoreUtilities/Core.h>

#include <gtest/gtest.h>

namespace UnitTests
{
	TEST(FixedSizeArenaAllocator, Reserve)
	{
		FixedSizeArenaAllocator<uint32_t> allocator;
		allocator.Reserve(100);

		ASSERT_EQ(allocator.GetNumMaxAllocations(), 100);
	}

	TEST(FixedSizeArenaAllocator, Allocate)
	{
		FixedSizeArenaAllocator<uint32_t> allocator;
		allocator.Reserve(100);

		ASSERT_EQ(allocator.GetNumMaxAllocations(), 100);

		const uint32_t numToAllocate = 10;
		
		for (uint32_t i = 0; i < numToAllocate; ++i)
		{
			uint32_t* alloc = allocator.Allocate();
			ASSERT_NE(alloc, nullptr);

			VT_UNUSED(alloc);
		}

		ASSERT_EQ(allocator.GetNumAllocations(), numToAllocate);
	}

	TEST(FixedSizeArenaAllocator, AllocateAndFree)
	{
		FixedSizeArenaAllocator<uint32_t> allocator;
		allocator.Reserve(100);

		ASSERT_EQ(allocator.GetNumMaxAllocations(), 100);

		const uint32_t numToAllocate = 10;

		Vector<uint32_t*> allocations;

		for (uint32_t i = 0; i < numToAllocate; ++i)
		{
			uint32_t* alloc = allocator.Allocate();
			ASSERT_NE(alloc, nullptr);

			allocations.emplace_back(alloc);
		}

		ASSERT_EQ(allocator.GetNumAllocations(), numToAllocate);

		for (uint32_t i = 0; i < numToAllocate; ++i)
		{
			ASSERT_TRUE(allocator.IsPointerWithinArena(allocations[i]));
			allocator.Free(allocations[i]);
		}

		ASSERT_EQ(allocator.GetNumAllocations(), 0);
	}

	TEST(FixedSizeArenaAllocator, Iterate)
	{
		FixedSizeArenaAllocator<uint32_t> allocator;
		allocator.Reserve(100);

		ASSERT_EQ(allocator.GetNumMaxAllocations(), 100);

		const uint32_t numToAllocate = 10;

		Vector<uint32_t*> allocations;

		// Allocate allocations.
		for (uint32_t i = 0; i < numToAllocate; ++i)
		{
			uint32_t* alloc = allocator.Allocate();
			ASSERT_NE(alloc, nullptr);

			allocations.emplace_back(alloc);
		}

		// Now we free a couple to get some space between the allocations.

		uint32_t numFreed = 0;
		for (uint32_t i = 0; i < numToAllocate; ++i)
		{
			if (i % 3)
			{
				allocator.Free(allocations[i]);
				numFreed++;
			}
		}

		const uint32_t numExpectedIterations = numToAllocate - numFreed;

		uint32_t numIterations = 0;
		for (FixedSizeArenaAllocator<uint32_t>::Iterator it(allocator); it; ++it)
		{
			ASSERT_NE(*it, nullptr);
			numIterations++;
		}

		ASSERT_EQ(numIterations, numExpectedIterations);
	}
}
