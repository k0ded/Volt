#pragma once

#include "CoreUtilities/Config.h"

#include "CoreUtilities/Allocators/LinearAllocator.h"


class VTCOREUTIL_API FrameStackAllocator
{
public:
	FrameStackAllocator();
	// The actual allocator
	struct VTCOREUTIL_API Mark
	{
		template<typename ValueType>
		class ForElementType
		{
		public:
			ForElementType()
				: m_allocation(nullptr)
			{}

			~ForElementType()
			{}


			VT_INLINE void* Allocate(size_t size, size_t alignment) noexcept
			{
				void* newAllocation = FrameStackAllocator::Get().AllocateOnStack(size, alignment);
				m_allocation = newAllocation;
				return newAllocation;
			}

			VT_INLINE void Free(void* allocation) noexcept
			{
				if (allocation == m_allocation)
				{
					m_allocation = nullptr;
				}
				return FrameStackAllocator::Get().FreeOnStack(allocation);
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

	void* AllocateOnStack(size_t size, size_t alignment);
	void FreeOnStack(void* ptr);
	void ClearStack();

	static FrameStackAllocator& Get();

private:
	inline static constexpr size_t FrameStackSize = 128 * 1024 * 1024; // 32MB

	LinearAllocator<DefaultHeapAllocator> m_linearAllocator;
	std::atomic_size_t m_numAllocations = 0;

};
