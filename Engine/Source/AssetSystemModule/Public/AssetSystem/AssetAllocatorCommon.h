#pragma once

#include "AssetSystem/Asset.h"
#include "AssetSystem/AssetManagerCommon.h"

#include <CoreUtilities/Allocators/PagedArenaAllocator.h>

namespace Volt
{
	class AssetTypeAllocator
	{
	public:
		virtual ~AssetTypeAllocator() = default;
		virtual void Free(void* allocation) = 0;
		virtual void Reallocate(void* allocation) = 0;
	};

	template<typename T>
	class AssetTypeAllocatorImpl : public AssetTypeAllocator
	{
	public:
		~AssetTypeAllocatorImpl() override = default;
		
		template<typename... Args>
		T* Allocate(Args&&... args)
		{
			return m_allocator.Allocate(std::forward<Args>(args)...);
		}

		void Reallocate(void* allocation) override
		{
			VT_MAYBE_UNUSED void* newAlloc = m_allocator.Reallocate(reinterpret_cast<T*>(allocation));
			VT_ENSURE(allocation == newAlloc);
		}

		void Free(void* allocation) override
		{
			m_allocator.Free(reinterpret_cast<T*>(allocation));
		}

	private:
		PagedArenaAllocator<T, 512> m_allocator;
	};
}
