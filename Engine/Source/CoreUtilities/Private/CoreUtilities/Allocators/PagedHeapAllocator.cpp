#include "cupch.h"

#include "CoreUtilities/Allocators/PagedHeapAllocator.h"
#include "CoreUtilities/MemoryUtility.h"
#include "CoreUtilities/Profiling/Profiling.h"

PagedHeapAllocator::PagedHeapAllocator()
{
}

PagedHeapAllocator::~PagedHeapAllocator()
{
	for (int32_t i = static_cast<int32_t>(m_pageAllocations.size()) - 1; i >= 0; --i)
	{
		FreePage(i);
	}
}

void* PagedHeapAllocator::Allocate(size_t size, size_t alignment)
{
	VT_PROFILE_FUNCTION();

	int32_t pageIndex = -1;
	size_t alignedSize = Utility::Align(size, std::max(MIN_PLATFORM_ALIGNMENT, alignment));

	{
		std::scoped_lock lock{ m_pagesMutex };
		for (size_t i = 0; i < m_pageAllocations.size(); ++i)
		{
			if (IsAllocationSupportedInPage(m_pageAllocations.at(i), alignedSize))
			{
				pageIndex = static_cast<int32_t>(i);
				break;
			}
		}
	}

	// No page was found, allocate a new one
	if (pageIndex == -1)
	{
		AllocateNewPage(alignedSize);
		pageIndex = static_cast<int32_t>(m_pageAllocations.size() - 1);
	}

	int32_t blockIndex = -1;

	std::scoped_lock lock{ m_pagesMutex };
	auto& page = m_pageAllocations.at(pageIndex);
	auto& availBlocks = page.availableBlocks;

	for (int32_t i = static_cast<int32_t>(availBlocks.size()) - 1; i >= 0; i--)
	{
		if (availBlocks.at(i).size >= alignedSize)
		{
			blockIndex = i;
			break;
		}
	}

	AllocationHeader allocationHeader{};

	if (blockIndex != -1)
	{
		AllocationBlock oldBlock = availBlocks.at(blockIndex);
		availBlocks.erase_unsorted(availBlocks.begin() + blockIndex);

		// If we didn't use the entire block, create a new available block with the smaller size
		if (oldBlock.size > alignedSize)
		{
			auto& newBlock = availBlocks.emplace_back();
			newBlock.size = oldBlock.size - alignedSize;
			newBlock.offset = oldBlock.offset + alignedSize;
		}

		allocationHeader.offset = oldBlock.offset;
		allocationHeader.size = alignedSize;
	}
	else
	{
		allocationHeader.offset = page.tail;
		allocationHeader.size = alignedSize;

		page.tail += alignedSize;
	}

	page.allocatedSize += alignedSize;
	allocationHeader.pageId = pageIndex;

	void* resultPtr = (reinterpret_cast<uint8_t*>(page.pointer) + allocationHeader.offset);
	m_allocationHeaderFromPointer[resultPtr] = allocationHeader;

	return resultPtr;
}

void PagedHeapAllocator::Free(void* pointer, size_t alignment)
{
	VT_PROFILE_FUNCTION();

	if (pointer == nullptr)
	{
		return;
	}

	std::scoped_lock lock{ m_pagesMutex };

	VT_ENSURE(m_allocationHeaderFromPointer.contains(pointer));

	AllocationHeader allocationHeader = m_allocationHeaderFromPointer.at(pointer);
	m_allocationHeaderFromPointer.erase(pointer);

	uint64_t finalOffset = allocationHeader.offset;
	uint64_t finalSize = allocationHeader.size;

	Vector<size_t, HeapAllocator> blocksToMerge;

	auto& page = m_pageAllocations.at(allocationHeader.pageId);

	for (size_t index = 0; const auto& block : page.availableBlocks)
	{
		if (block.offset + block.size == finalOffset)
		{
			finalOffset = block.offset;
			finalSize += block.size;
			blocksToMerge.emplace_back(index);
		}
		else if (block.offset == finalOffset + finalSize)
		{
			finalSize += block.size;
			blocksToMerge.emplace_back(index);
		}

		index++;
	}

	for (int32_t i = static_cast<int32_t>(blocksToMerge.size()) - 1; i >= 0; i--)
	{
		page.availableBlocks.erase_unsorted(page.availableBlocks.begin() + blocksToMerge.at(i));
	}

	uint64_t finalEndOffset = finalOffset + finalSize;
	if (finalEndOffset == page.tail)
	{
		page.tail = finalOffset;
	}
	else
	{
		page.availableBlocks.emplace_back(finalSize, finalOffset);
	}

	page.allocatedSize -= allocationHeader.size;
}

void PagedHeapAllocator::AllocateNewPage(uint64_t minSize)
{
	std::scoped_lock lock{ m_pagesMutex };

	auto& newPage = m_pageAllocations.emplace_back();

	// Allocate a new default page
	if (minSize < PageSize)
	{
		newPage.pointer = malloc(PageSize);
		newPage.size = PageSize;
	}
	// Allocate a custom fitted page
	else
	{
		newPage.pointer = malloc(minSize);
		newPage.size = minSize;
	}

	VT_PROFILE_ALLOC(newPage.pointer, newPage.size);
}

void PagedHeapAllocator::FreePage(size_t pageIndex)
{
	VT_ENSURE(pageIndex < m_pageAllocations.size());
	
	std::scoped_lock lock{ m_pagesMutex };
	auto& page = m_pageAllocations.at(pageIndex);
	
	VT_ENSURE_MSG(page.tail == 0, "All allocations must be released from page before freeing the page!");
	free(page.pointer);

	VT_PROFILE_FREE(page.pointer);

	m_pageAllocations.erase(m_pageAllocations.begin() + pageIndex);
}

bool PagedHeapAllocator::IsAllocationSupportedInPage(const PageAllocation& page, uint64_t size) const
{
	if (page.GetRemainingSize() < size)
	{
		return false;
	}

	if (page.GetRemainingTailSize() >= size)
	{
		return true;
	}

	for (const auto& availBlock : page.availableBlocks)
	{
		if (availBlock.size >= size)
		{
			return true;
		}
	}

	return false;
}
