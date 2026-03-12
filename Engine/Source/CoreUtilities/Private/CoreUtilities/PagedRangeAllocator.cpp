#include "cupch.h"

#include "CoreUtilities/RangeAllocator.h"
#include "PagedRangeAllocator.h"

void PagedRangeAllocator::SetPageSize(uint64_t pageSize)
{
	m_pageSize = pageSize;
}

PagedAllocatedRange PagedRangeAllocator::Allocate(uint64_t size)
{
	VT_ENSURE(size < m_pageSize);

	// Try to find a free range
	for (int32_t rangeIndex = static_cast<int32_t>(m_freeRanges.size()) - 1; rangeIndex >= 0; --rangeIndex)
	{
		PagedAllocatedRange& range = m_freeRanges[rangeIndex];

		// Found suitable range, shrink or remove.
		if (range.size >= size)
		{
			PagedAllocatedRange resultRange{};
			resultRange.offset = range.offset;
			resultRange.size = size;
			resultRange.pageIndex = range.pageIndex;

			range.offset += size;
			range.size -= size;

			if (range.size == 0)
			{
				m_freeRanges.erase_unsorted(m_freeRanges.begin() + rangeIndex);
			}

			return resultRange;
		}
	}

	// No free range found, allocate available page head.
	uint32_t pageIndex = 0xFFFFFFFF;
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_pageHeads.size()); ++i)
	{
		if (m_pageHeads[i] + size <= m_pageSize)
		{
			pageIndex = i;
			break;
		}
	}

	PagedAllocatedRange resultRange{};
	resultRange.size = size;

	if (pageIndex != 0xFFFFFFFF)
	{
		resultRange.pageIndex = pageIndex;
		resultRange.offset = m_pageHeads[pageIndex];
		m_pageHeads[pageIndex] += size;
	}
	else
	{
		resultRange.pageIndex = static_cast<uint32_t>(m_pageHeads.size());
		resultRange.offset = 0;

		uint64_t& newHead = m_pageHeads.emplace_back();
		newHead = size;
	}

	return resultRange;
}

void PagedRangeAllocator::Free(const PagedAllocatedRange& range)
{
	m_freeRanges.emplace_back(range);
}

void PagedRangeAllocator::MergeFreeAllocations()
{
	// Sort ranges by increasing offset
	std::sort(m_freeRanges.begin(), m_freeRanges.end(), [](const PagedAllocatedRange& lhs, const PagedAllocatedRange& rhs)
	{
		if (lhs.pageIndex != rhs.pageIndex)
		{
			return lhs.pageIndex < rhs.pageIndex;
		}

		return lhs.offset < rhs.offset;
	});

	for (int32_t rangeIndex = static_cast<int32_t>(m_freeRanges.size()) - 1; rangeIndex > 0; --rangeIndex)
	{
		PagedAllocatedRange& VT_RESTRICT nextRange = m_freeRanges[rangeIndex - 1];
		PagedAllocatedRange& VT_RESTRICT currRange = m_freeRanges[rangeIndex];

		// If the ranges lie next to each other merge them.
		if (nextRange.pageIndex == currRange.pageIndex && 
			nextRange.offset + nextRange.size == currRange.size)
		{
			nextRange.size += currRange.size;
			m_freeRanges.erase_unsorted(m_freeRanges.begin() + rangeIndex);
		}
	}
}
