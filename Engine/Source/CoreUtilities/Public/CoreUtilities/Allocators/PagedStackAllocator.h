#pragma once

#include "CoreUtilities/Allocators/ContainerAllocators.h"
#include "CoreUtilities/MemoryUtility.h"
#include "CoreUtilities/VoltAssert.h"

template<uint64_t MinPageSize>
class PagedStackAllocator
{
public:
	PagedStackAllocator() = default;
	~PagedStackAllocator()
	{
		PageHeader* currentPage = m_basePage;

		// No pages have been allocated.
		if (currentPage == nullptr)
		{
			return;
		}

		// Find the last page.
		while (currentPage->next != nullptr)
		{
			currentPage = currentPage->next;
		}

		// Walk backwards and free the pages along the ways
		while (currentPage->prev != nullptr)
		{
			PageHeader* tempPage = currentPage;
			currentPage = currentPage->prev;

			FreePage(tempPage);
		}

		// Finally free the base page.
		FreePage(m_basePage);
	}

	void* Allocate(uint64_t allocationSize, uint64_t alignment)
	{
		allocationSize = Utility::Align(allocationSize, alignment);

		if (!m_basePage)
		{
			m_basePage = AllocatePage(allocationSize);
		}

		PageHeader* currentPage = m_basePage;

		void* allocation = nullptr;

		while (true)
		{
			if (currentPage->TryAllocate(allocationSize, allocation))
			{
				break;
			}
			else
			{
				if (currentPage->next == nullptr)
				{
					PageHeader* newPage = AllocatePage(allocationSize);
					newPage->prev = currentPage;
					currentPage->next = newPage;
				}

				currentPage = currentPage->next;
			}
		}

		m_stackPointer += allocationSize;
		return allocation;
	}

	void ResetStackPointerTo(uint64_t value)
	{
		VT_ASSERT(value <= m_stackPointer);

		if (value == m_stackPointer)
		{
			return;
		}

		// Find current page
		if (!VT_CHECK(m_basePage != nullptr))
		{
			return;
		}

		PageHeader* lastRelevantPage = m_basePage;
		uint64_t lastRelevantPageOffset = 0;

		while (lastRelevantPage->dataPointer > 0 && lastRelevantPage->next)
		{
			lastRelevantPageOffset += lastRelevantPage->dataPointer;
			lastRelevantPage = lastRelevantPage->next;
		}

		while (true)
		{
			// Value is within current page
			if (value >= lastRelevantPageOffset)
			{
				const uint64_t diff = (lastRelevantPageOffset + lastRelevantPage->dataPointer) - value;
				lastRelevantPage->dataPointer -= diff;
			
				break;
			}
			else
			{
				lastRelevantPage->dataPointer = 0;
				lastRelevantPage = lastRelevantPage->prev;
				lastRelevantPageOffset -= lastRelevantPage->dataPointer;
			}
		}
		
		m_stackPointer = value;
	}

	VT_INLINE uint64_t GetStackPointer() const { return m_stackPointer; }

private:
	struct PageHeader
	{
		PageHeader* next = nullptr;
		PageHeader* prev = nullptr;
		uint64_t dataPointer = 0;
		uint64_t pageSize = 0;

		VT_INLINE uint8_t* GetData()
		{
			return reinterpret_cast<uint8_t*>(this) + sizeof(PageHeader);
		}

		VT_INLINE uint64_t GetDataSize()
		{
			return pageSize - sizeof(PageHeader);
		}

		VT_INLINE uint64_t GetAvailableSize()
		{
			return dataPointer >= GetDataSize() ? 0 : GetDataSize() - dataPointer;
		}

		bool TryAllocate(uint64_t allocationSize, void*& outDataPtr)
		{
			if (GetAvailableSize() < allocationSize)
			{
				return false;
			}

			uint64_t dataOffset = dataPointer;
			dataPointer += allocationSize;

			outDataPtr = GetData() + dataOffset;
			return true;
		}
	};

	PageHeader* AllocatePage(uint64_t allocationSize)
	{
		allocationSize = std::max(allocationSize, MinPageSize) + sizeof(PageHeader);

		uint8_t* newPage = reinterpret_cast<uint8_t*>(Memory::Malloc(allocationSize));
		PageHeader* pagePtr = new(newPage) PageHeader();
		pagePtr->pageSize = allocationSize;

		return pagePtr;
	}

	VT_INLINE void FreePage(PageHeader* page)
	{
		page->~PageHeader();
		Memory::Free(page);
	}

	PageHeader* m_basePage = nullptr;
	uint64_t m_stackPointer = 0;
};
