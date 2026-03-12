#pragma once

#include "CoreUtilities/Allocators/ContainerAllocators.h"
#include "CoreUtilities/Locks/SpinMutex.h"
#include "CoreUtilities/Locks/ScopedLock.h"

#include "CoreUtilities/VoltAssert.h"

template<uint64_t PageSize, typename SecondaryAllocator = DefaultHeapAllocator>
class PagedAtomicLinearAllocator
{
public:
	PagedAtomicLinearAllocator() = default;

	~PagedAtomicLinearAllocator()
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

	PagedAtomicLinearAllocator(PagedAtomicLinearAllocator&& other) noexcept
	{
		m_allocator = std::move(other.m_allocator);
		m_basePage.store(other.m_basePage);

		other.m_basePage = nullptr;
	}

	PagedAtomicLinearAllocator& operator=(PagedAtomicLinearAllocator&& other) noexcept
	{
		m_allocator = std::move(other.m_allocator);
		m_basePage.store(other.m_basePage);

		other.m_basePage = nullptr;
		return *this;
	}

	void ReservePages(uint32_t numPages)
	{
		PageHeader* lastPage = nullptr;

		uint32_t numPagesToAllocate = numPages;

		// If there are no pages allocated yet, we allocate the base page
		if (!m_basePage)
		{
			m_basePage = AllocatePage();
			lastPage = m_basePage;

			// Remove one because we have allocated the base page.
			numPagesToAllocate -= 1;
		}
		// Otherwise we count the number of pages currently allocated,
		// to figure out how many we need to allocate.
		else
		{
			uint32_t numCurrentPages = GetNumAllocatedPages();
			numPagesToAllocate = numCurrentPages >= numPagesToAllocate ? 0 : numPagesToAllocate - numCurrentPages;
		}

		VT_ENSURE(lastPage->next == nullptr);

		// Allocate the missing pages (if there are any)
		for (uint32_t i = 0; i < numPagesToAllocate; ++i)
		{
			lastPage->next = AllocatePage();
			lastPage->next.load()->prev = lastPage;
			lastPage = lastPage->next;
		}
	}

	void* Allocate(size_t allocationSize)
	{
		VT_ENSURE(allocationSize < PageSize, "An allocation must fit within a single page!");

		PageHeader* NullHeader = nullptr;

		if (!m_basePage)
		{
			// No base page has been allocated, try to allocate one and store
			// in the pointer.
			PageHeader* newPage = AllocatePage();
			if (!m_basePage.compare_exchange_strong(NullHeader, newPage, std::memory_order::release))
			{
				// Another thread already allocated it, free the page again.
				FreePage(newPage);
			}
		}

		PageHeader* currentPage = m_basePage.load(std::memory_order::acquire);

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
					// Try to allocate a new page
					PageHeader* newPage = AllocatePage();
					if (!currentPage->next.compare_exchange_strong(NullHeader, newPage, std::memory_order::release))
					{
						// Another thread already allocated it, free the page again.
						FreePage(newPage);
					}
					else
					{
						newPage->prev = currentPage;
					}
				}

				currentPage = currentPage->next.load(std::memory_order::acquire);
			}
		}

		return allocation;
	}

	uint32_t GetNumAllocatedPages() const
	{
		if (m_basePage == nullptr)
		{
			return 0;
		}

		uint32_t numCurrentPages = 1;
		PageHeader* currentPage = m_basePage;
		while (currentPage->next != nullptr)
		{
			numCurrentPages++;
			currentPage = currentPage->next;
		}

		return numCurrentPages;
	}

	/*
		Note: This is not a thread safe operation and should be used with caution!
	*/
	void Reset()
	{
		PageHeader* currentPage = m_basePage.load(std::memory_order::relaxed);

		while (currentPage != nullptr)
		{
			currentPage->dataPointer = 0;
			currentPage = currentPage->next.load(std::memory_order::relaxed);
		}
	}

private:
	struct PageHeader
	{
		std::atomic<PageHeader*> next = nullptr;
		PageHeader* prev = nullptr;
		std::atomic_uint64_t dataPointer = 0;

		uint8_t* GetData()
		{
			return reinterpret_cast<uint8_t*>(this) + sizeof(PageHeader);
		}

		constexpr uint64_t GetDataSize()
		{
			return PageSize - sizeof(PageHeader);
		}

		uint64_t GetAvailableSize()
		{
			return dataPointer >= GetDataSize() ? 0 : GetDataSize() - dataPointer;
		}

		bool TryAllocate(size_t allocationSize, void*& outDataPtr)
		{
			// Allocation won't fit, no need to try.
			if (GetAvailableSize() < allocationSize)
			{
				return false;
			}

			uint64_t dataOffset = dataPointer.fetch_add(allocationSize, std::memory_order::relaxed);

			// Another thread allocated before us, and we ended up out of range.
			if (dataOffset + allocationSize > GetDataSize())
			{
				return false;
			}

			outDataPtr = GetData() + dataOffset;
			return true;
		}
	};

	PageHeader* AllocatePage()
	{
		uint8_t* newPage = reinterpret_cast<uint8_t*>(m_allocator.Allocate(PageSize + sizeof(PageHeader), 0));
		return new(newPage) PageHeader();
	}

	void FreePage(PageHeader* page)
	{
		page->~PageHeader();
		m_allocator.Free(page);
	}

	SecondaryAllocator::template ForElementType<uint8_t> m_allocator;

	std::atomic<PageHeader*> m_basePage;

	public:
		class PageIterator
		{
		public:
			PageIterator()
			{}

			PageIterator(const PagedAtomicLinearAllocator& linearAllocator)
				: m_linearAllocator(&linearAllocator)
			{
				m_currentPage = m_linearAllocator->m_basePage;
			}

			VT_INLINE void operator++()
			{
				m_currentPage = m_currentPage->next.load(std::memory_order::relaxed);
			}

			VT_INLINE uint8_t* operator->() const
			{
				return m_currentPage->GetData();
			}

			VT_INLINE uint8_t* operator*() const
			{
				return m_currentPage->GetData();
			}

			VT_INLINE uint64_t Size() const
			{
				return m_currentPage->dataPointer.load(std::memory_order::relaxed);
			}

			VT_INLINE explicit operator bool() const
			{
				return m_linearAllocator != nullptr && m_currentPage != nullptr;
			}

		private:
			PageHeader* m_currentPage = nullptr;
			const PagedAtomicLinearAllocator* m_linearAllocator;
		};
};
