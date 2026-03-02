#pragma once

#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Config.h"

/*
 * Allows range allocation within any type of paged allocation.
 * Does not allocate any actual memory.
*/

struct PagedAllocatedRange
{
	uint64_t size;
	uint64_t offset;
	uint32_t pageIndex;
};

class PagedRangeAllocator
{
public:
	VTCOREUTIL_API void SetPageSize(uint64_t pageSize);
	VTCOREUTIL_API PagedAllocatedRange Allocate(uint64_t size);
	VTCOREUTIL_API void Free(const PagedAllocatedRange& range);
	VTCOREUTIL_API void MergeFreeAllocations();

	VT_NODISCARD VT_INLINE uint32_t GetNumPages() const { return static_cast<uint32_t>(m_pageHeads.size()); }

private:
	Vector<PagedAllocatedRange> m_freeRanges;
	Vector<uint64_t> m_pageHeads;

	uint64_t m_pageSize = 0;
};
