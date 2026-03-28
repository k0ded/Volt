#pragma once

#include "CoreUtilities/Allocators/ContainerAllocators.h"
#include "CoreUtilities/Containers/Array.h"
#include "CoreUtilities/Math/Math.h"

#include <cstdint>

template<typename Type, uint64_t PageSize, typename SecondaryAllocator = DefaultHeapAllocator>
class PagedAtomicArenaAllocator
{
public:
	PagedAtomicArenaAllocator() = default;
	~PagedAtomicArenaAllocator();

	PagedAtomicArenaAllocator(const PagedAtomicArenaAllocator& other) = delete;
	PagedAtomicArenaAllocator& operator=(const PagedAtomicArenaAllocator& other) = delete;

	PagedAtomicArenaAllocator(PagedAtomicArenaAllocator&& other) noexcept;
	PagedAtomicArenaAllocator& operator=(PagedAtomicArenaAllocator&& other) noexcept;

	template<typename... Args>
	Type* Allocate(Args&&... args);
	void Free(Type* allocation);

	void ReservePages(uint32_t numPages);
	uint64_t GetNumAllocatedPages() const;

private:
	struct SimpleBitset
	{
		using BitmaskType = uint64_t;

		inline static constexpr uint64_t NumBits = std::numeric_limits<BitmaskType>::digits;
		inline static constexpr uint64_t NumBitmasks = Math::DivideRoundUp(PageSize, NumBits);

		bool TrySetBit(uint64_t index);
		bool Test(uint64_t index);
		void ResetBit(uint64_t index);

		Array<std::atomic<BitmaskType>, NumBitmasks> bitset{};
	};

	struct Page
	{
		template<typename... Args>
		Type* TryAllocate(Args&&... args);
		void Free(Type* allocation);
		bool IsPointerWithinPage(Type* allocation);

		alignas(alignof(Type)) uint8_t data[PageSize * sizeof(Type)];
		SimpleBitset bitset;

		std::atomic<Page*> next = nullptr;
		std::atomic<uint64_t> nextIndex = 0;
	};

	void Release();

	Page* AllocatePage();
	void FreePage(Page* page);
	Page* GetOrAllocateBasePage();
	Page* GetOrAllocateNextPage(Page* current);

	std::atomic<Page*> m_basePage = nullptr;
	SecondaryAllocator::template ForElementType<uint8_t> m_allocator;

public:
	class Iterator
	{
	public:
		Iterator()
			: m_allocator(nullptr),
			m_currentPage(nullptr)
		{}

		Iterator(const PagedAtomicArenaAllocator& allocator)
			: m_allocator(&allocator),
			m_currentPage(nullptr)
		{
			// Find first allocation
			m_currentPage = m_allocator->m_basePage.load(std::memory_order::acquire);
			if (m_currentPage != nullptr)
			{
				Advance();
			}
		}

		VT_INLINE Iterator& operator++()
		{
			Advance();
			return *this;
		}

		VT_INLINE Type* operator->() const
		{
			VT_ASSERT(m_currentPage != nullptr && m_currentIndex < PageSize);
			Type* value = std::launder(reinterpret_cast<Type*>(&m_currentPage->data[sizeof(Type) * m_currentIndex]));
			return value;
		}

		VT_INLINE Type* operator*() const
		{
			VT_ASSERT(m_currentPage != nullptr && m_currentIndex < PageSize);
			Type* value = std::launder(reinterpret_cast<Type*>(&m_currentPage->data[sizeof(Type) * m_currentIndex]));
			return value;
		}

		VT_INLINE explicit operator bool() const
		{
			return m_allocator != nullptr && m_currentPage != nullptr;
		}

	private:
		void Advance();

		Page* m_currentPage;
		const PagedAtomicArenaAllocator* m_allocator;
		uint64_t m_currentIndex = std::numeric_limits<uint64_t>::max();
	};
};

#include "CoreUtilities/Allocators/PagedAtomicArenaAllocator.inl"
