#pragma once

#include "CoreUtilities/Profiling/Profiling.h"
#include "CoreUtilities/VoltAssert.h"

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::~PagedAtomicArenaAllocator()
{
	Release();
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::PagedAtomicArenaAllocator(PagedAtomicArenaAllocator&& other) noexcept
{
	m_basePage.store(other.m_basePage.exchange(nullptr, std::memory_order::relaxed), std::memory_order::relaxed);
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>& PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::operator=(PagedAtomicArenaAllocator&& other) noexcept
{
	if (this != &other)
	{
		Release();
		m_basePage.store(other.m_basePage.exchange(nullptr, std::memory_order::relaxed), std::memory_order::relaxed);
	}

	return *this;
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
template<typename... Args>
Type* PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::Allocate(Args&&... args)
{
	while (true)
	{
		Page* page = GetOrAllocateBasePage();
	
		while (page != nullptr)
		{
			Type* allocation = page->TryAllocate(std::forward<Args>(args)...);
			if (allocation != nullptr)
			{
				return allocation;
			}

			page = GetOrAllocateNextPage(page);
		}

		VT_ENSURE(false);
	}
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
void PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::Free(Type* allocation)
{
	VT_ENSURE(allocation != nullptr);

	Page* page = m_basePage.load(std::memory_order::acquire);
	while (page != nullptr)
	{
		if (page->IsPointerWithinPage(allocation))
		{
			page->Free(allocation);
			return;
		}

		page = page->next.load(std::memory_order::acquire);
	}

	VT_ENSURE_MSG(false, "Allocation not found in any page!");
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
uint64_t PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::GetNumAllocatedPages() const
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

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
void PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::ReservePages(uint32_t numPages)
{
	const uint64_t numAllocatedPages = GetNumAllocatedPages();
	if (numAllocatedPages >= numPages)
	{
		return;
	}

	Page* currentPage = GetOrAllocateBasePage();
	
	for (uint64_t i = 1; i < numPages; ++i)
	{
		currentPage = GetOrAllocateNextPage(currentPage);
	}
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
void PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::Release()
{
	Page* page = m_basePage.exchange(nullptr, std::memory_order::acq_rel);
	while (page != nullptr)
	{
		Page* next = page->next.load(std::memory_order::relaxed);
		FreePage(page);
		page = next;
	}
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::Page* PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::GetOrAllocateBasePage()
{
	Page* page = m_basePage.load(std::memory_order::acquire);
	if (page != nullptr)
	{
		return page;
	}

	Page* newPage = AllocatePage();
	if (!m_basePage.compare_exchange_strong(page, newPage,
		std::memory_order::release,
		std::memory_order::acquire))
	{
		FreePage(newPage);
	}

	return m_basePage.load(std::memory_order::acquire);
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::Page* PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::GetOrAllocateNextPage(Page* current)
{
	Page* next = current->next.load(std::memory_order::acquire);
	if (next != nullptr)
	{
		return next;
	}

	Page* newPage = AllocatePage();
	if (!current->next.compare_exchange_strong(next, newPage,
		std::memory_order::release,
		std::memory_order::acquire))
	{
		FreePage(newPage);
		return current->next.load(std::memory_order::acquire);
	}

	return newPage;
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::Page* PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::AllocatePage()
{
	void* newPtr = m_allocator.Allocate(sizeof(Page), 0);
	return new(newPtr) Page();
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
void PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::FreePage(Page* page)
{
	if constexpr (AllocatorOwnsAllocations)
	{
		uint64_t index = 0;

		for (uint64_t bitmask : page->bitset.bitset)
		{
			for (uint64_t bit = 0; bit < SimpleBitset::NumBits; ++bit)
			{
				if ((bitmask & (1ull << bit)) != 0)
				{
					Type* object = std::launder(reinterpret_cast<Type*>(&page->data[index * sizeof(Type)]));
					object->~Type();
				}
				index++;
			}
		}
	}
	else
	{
		for (uint64_t bitmask : page->bitset.bitset)
		{
			VT_ENSURE_MSG(bitmask == 0, "Not all entries were destroyed prior to destruction!");
		}
	}

	page->~Page();
	m_allocator.Free(page);
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
template<typename... Args>
Type* PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::Page::TryAllocate(Args&&... args)
{
	VT_PROFILE_FUNCTION();

	const uint64_t nextSearchIndex = nextIndex.load(std::memory_order::relaxed);
	for (uint64_t i = 0; i < PageSize; ++i)
	{
		const uint64_t index = (nextSearchIndex + i) % PageSize;
		const bool success = bitset.TrySetBit(index);

		// Successfully grabbed this slot.
		if (success)
		{
			void* ptr = &data[index * sizeof(Type)];
			Type* newObject = new(ptr) Type(std::forward<Args>(args)...);

			const uint64_t newSearchIndex = (index + 1) % PageSize;
			nextIndex.store(newSearchIndex, std::memory_order::relaxed);

			return newObject;
		}
	}

	return nullptr;
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
void PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::Page::Free(Type* allocation)
{
	std::ptrdiff_t allocationIndex = allocation - reinterpret_cast<Type*>(data);
	allocation->~Type();

	bitset.ResetBit(allocationIndex);
	nextIndex.store(allocationIndex, std::memory_order::relaxed);
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
bool PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::SimpleBitset::TrySetBit(uint64_t index)
{
	const BitmaskType bitmaskIndex = index / NumBits;
	const BitmaskType bitIndex = index % NumBits;

	const BitmaskType mask = (BitmaskType(1) << bitIndex);

	const BitmaskType prevValue = bitset[bitmaskIndex].fetch_or(mask, std::memory_order::relaxed);

	// If bit wasn't set before, we got this slot.
	return (prevValue & mask) == 0;
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
bool PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::SimpleBitset::Test(uint64_t index)
{
	const BitmaskType bitmaskIndex = index / NumBits;
	const BitmaskType bitIndex = index % NumBits;

	const BitmaskType mask = (BitmaskType(1) << bitIndex);

	const BitmaskType bitmask = bitset[bitmaskIndex].load(std::memory_order::relaxed);

	return (bitmask & mask) != 0;
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
void PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::SimpleBitset::ResetBit(uint64_t index)
{
	const BitmaskType bitmaskIndex = index / NumBits;
	const BitmaskType bitIndex = index % NumBits;

	const BitmaskType mask = (BitmaskType(1) << bitIndex);

	VT_MAYBE_UNUSED const BitmaskType prevValue = bitset[bitmaskIndex].fetch_and(~mask, std::memory_order::relaxed);
	VT_ASSERT((prevValue & mask) != 0);
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
bool PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::Page::IsPointerWithinPage(Type* allocation)
{
	const Type* dataPtr = reinterpret_cast<Type*>(data);
	return allocation >= dataPtr && allocation < (dataPtr + PageSize);
}

template<typename Type, uint64_t PageSize, typename SecondaryAllocator /*= DefaultHeapAllocator*/, bool AllocatorOwnsAllocations /*= false*/>
void PagedAtomicArenaAllocator<Type, PageSize, SecondaryAllocator, AllocatorOwnsAllocations>::Iterator::Advance()
{
	while (m_currentPage != nullptr)
	{
		bool found = false;

		const uint64_t startIndex = m_currentIndex == std::numeric_limits<uint64_t>::max() ? 0 : m_currentIndex + 1;
		for (uint64_t i = startIndex; i < PageSize; ++i)
		{
			if (m_currentPage->bitset.Test(i))
			{
				found = true;
				m_currentIndex = i;
				break;
			}
		}

		if (found)
		{
			break;
		}
		else
		{
			m_currentPage = m_currentPage->next.load(std::memory_order::relaxed);
			m_currentIndex = std::numeric_limits<uint64_t>::max();
		}
	}
}
