#pragma once

#include "CoreUtilities/Malloc.h"
#include "CoreUtilities/Memory.h"

class DefaultHeapAllocator
{
public:
	template<typename ValueType>
	class ForElementType
	{
	public:
		ForElementType()
			: m_allocation(nullptr)
		{}

		~ForElementType()
		{}

		VT_INLINE void* Allocate(size_t size, size_t alignment)
		{
			void* newAllocation = Memory::Malloc(size, alignment);
			m_allocation = newAllocation;
			return newAllocation;
		}

		VT_INLINE void Free(void* allocation)
		{
			if (allocation == m_allocation)
			{
				m_allocation = nullptr;
			}
			Memory::Free(allocation);
		}

		VT_INLINE void Swap(ForElementType& other)
		{
			std::swap(m_allocation, other.m_allocation);
		}

		VT_INLINE ValueType* GetAllocation() { return reinterpret_cast<ValueType*>(m_allocation); }

	private:
		void* m_allocation;
	};
};

template<size_t NumValues, typename SecondaryAllocator = DefaultHeapAllocator>
class InlineAllocator
{
public:
	template<typename ValueType>
	class ForElementType
	{
	public:
		ForElementType() = default;

		ForElementType(const ForElementType& other) noexcept
		{
			if (!other.m_heapAllocation)
			{
				memcpy_s(m_data, TotalSize, other.m_data, TotalSize);
			}
		}

		ForElementType(ForElementType&& other) noexcept
		{
			if (!other.m_heapAllocation)
			{
				memmove_s(m_data, TotalSize, other.m_data, TotalSize);
			}
		}

		ForElementType& operator=(const ForElementType& other) noexcept
		{
			if (&other != this)
			{
				memcpy_s(m_data, TotalSize, other.m_data, TotalSize);
			}
			return *this;
		}

		ForElementType& operator=(ForElementType&& other) noexcept
		{
			if (&other != this)
			{
				memmove_s(m_data, TotalSize, other.m_data, TotalSize);
			}
			return *this;
		}

		void* Allocate(size_t size, size_t alignment) noexcept
		{
			// Allocate on heap
			if (size > TotalSize)
			{
				void* newAllocation = m_allocator.Allocate(size, alignment);
				m_heapAllocation = newAllocation;
				return newAllocation;
			}

			// Make sure the heap pointer is null.
			// The pointer will be freed by a call from the container.
			m_heapAllocation = nullptr;

			// Local storage
			return m_data;
		}

		void Free(void* allocation) noexcept
		{
			// Allocation is not the local storage,
			// let's free it.
			if (allocation != m_data)
			{
				if (allocation == m_heapAllocation)
				{
					m_heapAllocation = nullptr;
				}
				m_allocator.Free(allocation);
			}
		}

		VT_INLINE void Swap(ForElementType& other)
		{
			std::swap(m_heapAllocation, other.m_heapAllocation);
			std::swap(m_data, other.m_data);
		}

		VT_INLINE ValueType* GetAllocation() { return reinterpret_cast<ValueType*>(m_data); }

	private:
		inline static constexpr size_t TotalSize = sizeof(ValueType) * NumValues;

		uint8_t m_data[TotalSize];
		void* m_heapAllocation = nullptr;
		SecondaryAllocator::template ForElementType<ValueType> m_allocator;
	};
};
