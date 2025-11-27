#pragma once

#include "CoreUtilities/Allocators/ContainerAllocators.h"

/*
	A non-thread safe linear allocator.
	Uses pages internally for allocation, allows larger-than page size allocations.
*/

template<uint64_t MinPageSize, typename SecondaryAllocator = DefaultHeapAllocator>
class LinearAllocator
{
public:
	LinearAllocator()
	{}

	~LinearAllocator()
	{
		PageHeader* currentPage = m_basePage;

		// No pages have been allocated
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

	LinearAllocator(LinearAllocator&& other) noexcept
	{
		m_basePage = other.m_basePage;
		other.m_basePage = nullptr;
	}

	LinearAllocator& operator=(LinearAllocator&& other) noexcept
	{
		if (this != &other)
		{
			m_basePage = other.m_basePage;
			other.m_basePage = nullptr;
		}

		return *this;
	}

	LinearAllocator(const LinearAllocator&) noexcept = delete;
	LinearAllocator& operator=(const LinearAllocator&) noexcept = delete;

	void* Allocate(uint64_t allocationSize)
	{
		if (m_basePage == nullptr)
		{
			m_basePage = AllocatePage(std::max(MinPageSize, allocationSize));
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
					currentPage->next = newPage;
					newPage->prev = currentPage;
				}

				currentPage = currentPage->next;
			}
		}

		return allocation;
	}

private:
	struct PageHeader
	{
		PageHeader* next = nullptr;
		PageHeader* prev = nullptr;
		uint64_t size = 0;
		uint64_t dataPointer = 0;

		uint8_t* GetData()
		{
			return reinterpret_cast<uint8_t*>(this) + sizeof(PageHeader);
		}

		constexpr uint64_t GetDataSize()
		{
			return size - sizeof(PageHeader);
		}

		uint64_t GetAvailableSize()
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

	PageHeader* AllocatePage(uint64_t pageSize)
	{
		const uint64_t allocationSize = pageSize + sizeof(PageHeader);

		uint8_t* newPageAlloc = reinterpret_cast<uint8_t*>(m_allocator.Allocate(allocationSize, 0));
		PageHeader* newPage = new(newPageAlloc) PageHeader();

		newPage->size = allocationSize;

		return newPage;
	}

	void FreePage(PageHeader* page)
	{
		page->~PageHeader();
		m_allocator.Free(page);
	}

	SecondaryAllocator::template ForElementType<uint8_t> m_allocator;
	PageHeader* m_basePage = nullptr;
};
