#include "cupch.h"

#include "CoreUtilities/RangeAllocator.h"

AllocatedRange RangeAllocator::Allocate(uint64_t size)
{
	// Try to find a free range
	for (int32_t rangeIndex = static_cast<int32_t>(m_freeRanges.size()) - 1; rangeIndex >= 0; --rangeIndex)
	{
		AllocatedRange& range = m_freeRanges[rangeIndex];
		
		// Found suitable range, shrink or remove.
		if (range.size >= size)
		{
			AllocatedRange resultRange{};
			resultRange.offset = range.offset;
			resultRange.size = size;

			range.offset += size;
			range.size -= size;

			if (range.size == 0)
			{
				m_freeRanges.erase_unsorted(m_freeRanges.begin() + rangeIndex);
			}

			return resultRange;
		}
	}

	// No free range found, allocate from head.
	AllocatedRange resultRange{};
	resultRange.offset = m_currentHead;
	resultRange.size = size;

	m_currentHead += size;

	return resultRange;
}

void RangeAllocator::Free(const AllocatedRange& range)
{
	m_freeRanges.emplace_back(range);
}

void RangeAllocator::MergeFreeAllocations()
{
	// Sort ranges by increasing offset
	std::sort(m_freeRanges.begin(), m_freeRanges.end(), [](const AllocatedRange& lhs, const AllocatedRange& rhs) 
	{
		return lhs.offset < rhs.offset;
	});

	for (int32_t rangeIndex = static_cast<int32_t>(m_freeRanges.size()) - 1; rangeIndex > 0; --rangeIndex)
	{
		AllocatedRange& VT_RESTRICT nextRange = m_freeRanges[rangeIndex - 1];
		AllocatedRange& VT_RESTRICT currRange = m_freeRanges[rangeIndex];

		// If the ranges lie next to each other merge them.
		if (nextRange.offset + nextRange.size == currRange.size)
		{
			nextRange.size += currRange.size;
			m_freeRanges.erase_unsorted(m_freeRanges.begin() +rangeIndex);
		}
	}
}
