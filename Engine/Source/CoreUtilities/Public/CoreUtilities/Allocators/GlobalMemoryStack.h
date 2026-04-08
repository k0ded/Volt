#pragma once

#include "CoreUtilities/Allocators/PagedStackAllocator.h"
#include "CoreUtilities/Config.h"

class GlobalMemoryStack
{
public:
	VTCOREUTIL_API void* Allocate(uint64_t numBytes, uint64_t alignment);

	VTCOREUTIL_API static GlobalMemoryStack& Get();

private:
	friend class GlobalMemoryStackMark;

	VTCOREUTIL_API void ResetStackPointerTo(uint64_t value);
	VT_INLINE uint64_t GetStackPointer() const { return m_allocator.GetStackPointer(); }

	PagedStackAllocator<65535> m_allocator;
	uint32_t m_markStackDepth = 0;
};

/*
* A helper that, in it's destructor, pops the stack back to where it was when it was created.
*/
class GlobalMemoryStackMark
{
public:
	GlobalMemoryStackMark()
	{
		m_stackPointer = GlobalMemoryStack::Get().GetStackPointer();
		GlobalMemoryStack::Get().m_markStackDepth++;
	}

	~GlobalMemoryStackMark()
	{
		GlobalMemoryStack::Get().ResetStackPointerTo(m_stackPointer);
		GlobalMemoryStack::Get().m_markStackDepth--;
	}

private:
	uint64_t m_stackPointer = 0;
};

class GlobalMemoryStackAllocator
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
			void* newAllocation = GlobalMemoryStack::Get().Allocate(size, alignment);
			m_allocation = newAllocation;
			return newAllocation;
		}

		VT_INLINE void Free(void* allocation)
		{}

		VT_INLINE void Swap(ForElementType& other)
		{
			std::swap(m_allocation, other.m_allocation);
		}

		VT_INLINE ValueType* GetAllocation() { return reinterpret_cast<ValueType*>(m_allocation); }

	private:
		void* m_allocation;
	};
};
