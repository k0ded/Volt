#pragma once

#include "CoreUtilities/Allocators/ArenaAllocator.h"

template<typename Type, uint64_t PageSize, typename SecondaryAllocator = DefaultHeapAllocator>
class PagedArenaAllocator
{
public:
	PagedArenaAllocator() = default;

	~PagedArenaAllocator()
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

	PagedArenaAllocator(PagedArenaAllocator&& other) noexcept
	{
		m_allocator = std::move(other.m_allocator);
		m_basePage.store(other.m_basePage);

		other.m_basePage = nullptr;
	}

	PagedArenaAllocator& operator=(PagedArenaAllocator&& other) noexcept
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

	template<typename... Args>
	Type* Allocate(Args&&... args)
	{
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

		Type* allocation = nullptr;

		while (true)
		{
			if (allocation = currentPage->TryAllocate(std::forward<Args>(args)...); allocation != nullptr)
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

	void Free(Type* allocation)
	{
		PageHeader* currentPage = m_basePage;
		while (currentPage != nullptr)
		{
			if (currentPage->arena.IsPointerWithinArena(allocation))
			{
				currentPage->arena.Free(allocation);
				break;
			}

			currentPage = currentPage->next;
		}
	}

	bool IsPointerWithinArena(Type* allocation) const
	{
		PageHeader* currentPage = m_basePage;
		while (currentPage != nullptr)
		{
			if (currentPage->arena.IsPointerWithinArena(allocation))
			{
				return true;
			}

			currentPage = currentPage->next;
		}

		return false;
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

private:
	struct PageHeader
	{
		std::atomic<PageHeader*> next = nullptr;
		PageHeader* prev = nullptr;
	
		ArenaAllocator<Type, SecondaryAllocator> arena;

		template<typename... Args>
		Type* TryAllocate(Args&&... args)
		{
			return arena.Allocate(std::forward<Args>(args)...);
		}
	};

	PageHeader* AllocatePage()
	{
		uint8_t* newPageDst = reinterpret_cast<uint8_t*>(m_allocator.Allocate(sizeof(PageHeader), 0));
		PageHeader* newPage = new(newPageDst) PageHeader();

		newPage->arena.Reserve(PageSize);

		return newPage;
	}

	void FreePage(PageHeader* page)
	{
		page->~PageHeader();
		m_allocator.Free(page);
	}

	std::atomic<PageHeader*> m_basePage = nullptr;
	SecondaryAllocator::template ForElementType<uint8_t> m_allocator;
};
