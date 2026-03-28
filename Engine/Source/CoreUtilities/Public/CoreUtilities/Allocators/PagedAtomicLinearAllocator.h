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
	~PagedAtomicLinearAllocator();

	PagedAtomicLinearAllocator(const PagedAtomicLinearAllocator& other) = delete;
	PagedAtomicLinearAllocator& operator=(const PagedAtomicLinearAllocator& other) = delete;

	PagedAtomicLinearAllocator(PagedAtomicLinearAllocator&& other) noexcept;
	PagedAtomicLinearAllocator& operator=(PagedAtomicLinearAllocator&& other) noexcept;

	void ReservePages(uint32_t numPages);
	void* Allocate(uint64_t allocationSize);

	uint32_t GetNumAllocatedPages() const;

	/*
		Note: This is not a thread safe operation and should be used with caution!
	*/
	void Reset();

private:
	struct Page
	{
		void* TryAllocate(uint64_t allocationSize);
		uint8_t* GetDataPtr();

		uint64_t pageSize;
		std::atomic<Page*> next = nullptr;
		std::atomic<uint64_t> dataPointer = 0;
	};

	void Release();

	Page* AllocatePage(uint64_t requiredSize);
	void FreePage(Page* page);
	Page* GetOrAllocateBasePage(uint64_t requiredSize);
	Page* GetOrAllocateNextPage(Page* current, uint64_t requiredSize);

	std::atomic<Page*> m_basePage;
	SecondaryAllocator::template ForElementType<uint8_t> m_allocator;

	public:
		class PageIterator
		{
		public:
			PageIterator()
			{}

			PageIterator(const PagedAtomicLinearAllocator& linearAllocator)
				: m_linearAllocator(&linearAllocator)
			{
				m_currentPage = m_linearAllocator->m_basePage.load(std::memory_order::acquire);
			}

			VT_INLINE void operator++()
			{
				m_currentPage = m_currentPage->next.load(std::memory_order::acquire);
			}

			VT_INLINE uint8_t* operator->() const
			{
				return m_currentPage->GetDataPtr();
			}

			VT_INLINE uint8_t* operator*() const
			{
				return m_currentPage->GetDataPtr();
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
			Page* m_currentPage = nullptr;
			const PagedAtomicLinearAllocator* m_linearAllocator;
		};
};

#include "PagedAtomicLinearAllocator.inl"
