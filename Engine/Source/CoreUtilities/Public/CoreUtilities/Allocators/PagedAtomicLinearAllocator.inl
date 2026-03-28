#include "PagedAtomicLinearAllocator.h"
#pragma once

template<uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/>
PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::~PagedAtomicLinearAllocator()
{
	Release();
}

template<uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/>
PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::PagedAtomicLinearAllocator(PagedAtomicLinearAllocator&& other) noexcept
{
	m_allocator = std::move(other.m_allocator);
	m_basePage.store(other.m_basePage.exchange(nullptr, std::memory_order::relaxed), std::memory_order::relaxed);
}

template<uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/>
PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>& PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::operator=(PagedAtomicLinearAllocator&& other) noexcept
{
	if (&other != this)
	{
		Release();

		m_allocator = std::move(other.m_allocator);
		m_basePage.store(other.m_basePage.exchange(nullptr, std::memory_order::relaxed), std::memory_order::relaxed);
	}

	return *this;
}

template<uint64_t PageSize, typename SecondaryAllocator>
inline void PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::ReservePages(uint32_t numPages)
{
	const uint64_t numAllocatedPages = GetNumAllocatedPages();
	if (numAllocatedPages >= numPages)
	{
		return;
	}

	Page* currentPage = GetOrAllocateBasePage(PageSize);

	for (uint64_t i = 1; i < numPages; ++i)
	{
		currentPage = GetOrAllocateNextPage(currentPage, PageSize);
	}
}

template<uint64_t PageSize, typename SecondaryAllocator>
inline void* PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::Allocate(uint64_t allocationSize)
{
	while (true)
	{
		Page* page = GetOrAllocateBasePage(allocationSize);

		while (page != nullptr)
		{
			void* allocation = page->TryAllocate(allocationSize);
			if (allocation != nullptr)
			{
				return allocation;
			}

			page = GetOrAllocateNextPage(page, allocationSize);
		}
	}
}

template<uint64_t PageSize, typename SecondaryAllocator>
inline uint32_t PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::GetNumAllocatedPages() const
{
	uint64_t result = 0;
	Page* page = m_basePage.load(std::memory_order::acquire);
	while (page != nullptr)
	{
		result++;
		page = page->next.load(std::memory_order::acquire);
	}

	return result;
}

template<uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/>
void PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::Reset()
{
	Page* currentPage = m_basePage.load(std::memory_order::acquire);
	while (currentPage != nullptr)
	{
		currentPage->dataPointer = 0;
		currentPage = currentPage->next.load(std::memory_order::acquire);
	}
}

template<uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/>
void PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::Release()
{
	Page* currentPage = m_basePage.exchange(nullptr, std::memory_order::acq_rel);
	while (currentPage != nullptr)
	{
		Page* next = currentPage->next.load(std::memory_order::relaxed);
		FreePage(currentPage);

		currentPage = next;
	}
}

template<uint64_t PageSize, typename SecondaryAllocator>
inline PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::Page* PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::AllocatePage(uint64_t requiredSize)
{
	const uint64_t newPageSize = std::max(PageSize, requiredSize);
	const uint64_t totalSize = newPageSize + sizeof(Page);

	void* dataPtr = m_allocator.Allocate(totalSize, 0);
	Page* pagePtr = new (dataPtr) Page();
	
	pagePtr->pageSize = newPageSize;

	return pagePtr;
}

template<uint64_t PageSize, typename SecondaryAllocator>
inline void PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::FreePage(Page* page)
{
	page->~Page();
	m_allocator.Free(page);
}

template<uint64_t PageSize, typename SecondaryAllocator>
inline PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::Page* PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::GetOrAllocateBasePage(uint64_t requiredSize)
{
	Page* page = m_basePage.load(std::memory_order::acquire);
	if (page != nullptr)
	{
		return page;
	}

	// Note: If required size is greater than PageSize and the thread fails this check, it will 
	//		 end up calling GetOrAllocateNextPage after failing to allocate from the base page.

	Page* newPage = AllocatePage(requiredSize);
	if (!m_basePage.compare_exchange_strong(page, newPage,
		std::memory_order::release,
		std::memory_order::acquire))
	{
		FreePage(newPage);
	}

	return m_basePage.load(std::memory_order::acquire);
}

template<uint64_t PageSize, typename SecondaryAllocator>
inline PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::Page* PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::GetOrAllocateNextPage(Page* current, uint64_t requiredSize)
{
	Page* next = current->next.load(std::memory_order::acquire);
	if (next != nullptr)
	{
		return next;
	}

	// Note: If required size is greater than PageSize and the thread fails this check, it will 
	//		 end up calling GetOrAllocateNextPage after failing to allocate from the new page.

	Page* newPage = AllocatePage(requiredSize);
	if (!current->next.compare_exchange_strong(next, newPage,
		std::memory_order::release,
		std::memory_order::acquire))
	{
		FreePage(newPage);
		return current->next.load(std::memory_order::acquire);
	}

	return newPage;
}

template<uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/>
uint8_t* PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::Page::GetDataPtr()
{
	return reinterpret_cast<uint8_t*>(this) + sizeof(Page);
}

template<uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/>
void* PagedAtomicLinearAllocator<PageSize, SecondaryAllocator>::Page::TryAllocate(uint64_t allocationSize)
{
	const uint64_t currentDataSize = dataPointer.load(std::memory_order::relaxed);
	if (currentDataSize + allocationSize <= pageSize)
	{
		const uint64_t offset = dataPointer.fetch_add(allocationSize, std::memory_order::release);
		if (offset + allocationSize <= pageSize)
		{
			return GetDataPtr() + offset;
		}
		else
		{
			// We lost the allocation race, remove our "allocation" again
			dataPointer.fetch_sub(allocationSize, std::memory_order::acquire);
		}
	}

	return nullptr;
}
