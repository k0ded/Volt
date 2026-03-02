#pragma once

#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Config.h"

/*
 * Allows range allocation within any type of allocation.
 * Does not allocate any actual memory.
*/

struct AllocatedRange
{
	uint64_t size;
	uint64_t offset;
};

class RangeAllocator
{
public:
	VTCOREUTIL_API AllocatedRange Allocate(uint64_t size);
	VTCOREUTIL_API void Free(const AllocatedRange& range);
	VTCOREUTIL_API void MergeFreeAllocations();

private:
	Vector<AllocatedRange> m_freeRanges;
	uint64_t m_currentHead = 0;
};
