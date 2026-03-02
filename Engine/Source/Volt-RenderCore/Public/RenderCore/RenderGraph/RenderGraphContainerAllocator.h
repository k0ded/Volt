#pragma once

#include "RenderCore/RenderGraph/RenderGraphDataAllocator.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/MemoryUtility.h>

namespace Volt
{
	class RenderGraphContainerAllocator
	{
	public:
		template<typename ValueType>
		class ForElementType
		{
		public:
			ForElementType()
				: m_allocation(nullptr),
				m_dataAllocator(nullptr)
			{}

			ForElementType(RenderGraphDataAllocator* dataAllocator)
				: m_allocation(nullptr),
				m_dataAllocator(dataAllocator)
			{}

			VT_INLINE void* Allocate(size_t size, size_t alignment)
			{
				constexpr size_t DefaultAlignment = 8;

				VT_ENSURE(m_dataAllocator != nullptr);

				if (alignment != DefaultAlignment)
				{
					alignment = std::max(size_t(size >= 16u ? 16u : 8u), alignment);
				}
				else
				{
					alignment = size_t(size >= 16u ? 16u : DefaultAlignment);
				}

				const size_t alignedSize = ::Utility::Align(size, alignment);
				void* resultPtr = m_dataAllocator->Allocate(alignedSize);
				m_allocation = resultPtr;

				return resultPtr;
			}

			VT_INLINE void Free(void* allocation)
			{}

			VT_INLINE void Swap(ForElementType& other)
			{
				std::swap(m_allocation, other.m_allocation);
				std::swap(m_dataAllocator, other.m_dataAllocator);
			}

			VT_INLINE ValueType* GetAllocation() { return reinterpret_cast<ValueType*>(m_allocation); }

			static void CopyAllocator(ForElementType& dst, const ForElementType& src)
			{
				dst.m_dataAllocator = src.m_dataAllocator;
			}

		private:
			void* m_allocation;
			RenderGraphDataAllocator* m_dataAllocator;
		};
	};

	template<typename T>
	using RGVector = Vector<T, RenderGraphContainerAllocator>;
}

template<>
struct ContainerAllocatorTraits<Volt::RenderGraphContainerAllocator>
{
	inline static constexpr bool RequiresAllocatorCopy = true;
};
