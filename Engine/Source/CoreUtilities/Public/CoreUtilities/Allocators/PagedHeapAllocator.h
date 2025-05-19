#pragma once

#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Containers/Map.h"

#include "CoreUtilities/Allocators/HeapAllocator.h"

class VTCOREUTIL_API PagedHeapAllocator
{
public:
	PagedHeapAllocator();
	~PagedHeapAllocator();

	void* Allocate(size_t size, size_t alignment);
	void Free(void* pointer, size_t alignment);

private:
	// A page is 64 MB to allow most allocations to fit, otherwise they will be custom fit
	inline static constexpr size_t PageSize = 64 * 1024 * 1024;

	struct AllocationHeader
	{
		size_t pageId;
		uint64_t offset;
		uint64_t size;
	};

	struct AllocationBlock
	{
		uint64_t size = 0;
		uint64_t offset = 0;
	};

	struct PageAllocation
	{
		void* pointer = nullptr;
		uint64_t size = 0;

		uint64_t allocatedSize = 0;
		uint64_t tail = 0;

		Vector<AllocationBlock, HeapAllocator> availableBlocks;

		VT_NODISCARD VT_INLINE const uint64_t GetRemainingSize() const
		{
			return size - std::min(allocatedSize, size);
		}

		VT_NODISCARD VT_INLINE const uint64_t GetRemainingTailSize() const
		{
			return size - tail;
		}
	};

	void AllocateNewPage(uint64_t minSize);
	void FreePage(size_t pageIndex);

	bool IsAllocationSupportedInPage(const PageAllocation& page, uint64_t size) const;

	std::mutex m_pagesMutex;
	Vector<PageAllocation, HeapAllocator> m_pageAllocations;
	vt::map<void*, AllocationHeader> m_allocationHeaderFromPointer;
};
