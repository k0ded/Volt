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

template<size_t NumValues>
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
			memcpy_s(m_data, TotalSize, other.m_data, TotalSize);
		}

		ForElementType(ForElementType&& other) noexcept
		{
			memmove_s(m_data, TotalSize, other.m_data, TotalSize);
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
			VT_ENSURE(size <= TotalSize);
			return m_data;
		}

		void Free(void* pointer) noexcept
		{

		}

		VT_INLINE void Swap(ForElementType& other)
		{
			std::swap(m_data, other.m_data);
		}

		VT_INLINE ValueType* GetAllocation() { return reinterpret_cast<ValueType*>(m_data); }

	private:
		inline static constexpr size_t TotalSize = sizeof(ValueType) * NumValues;

		uint8_t m_data[TotalSize];
	};
};

