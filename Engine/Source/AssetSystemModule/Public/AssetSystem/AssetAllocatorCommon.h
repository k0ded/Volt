#pragma once

#include "AssetSystem/Asset_New.h"
#include "AssetSystem/AssetManagerCommon.h"

#include <CoreUtilities/Allocators/PagedArenaAllocator.h>

namespace Volt
{
	class AssetTypeAllocator
	{
	public:
		virtual ~AssetTypeAllocator() = default;
		virtual void Free(void* allocation) = 0;
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

		void Free(void* allocation) override
		{
			m_allocator.Free(reinterpret_cast<T*>(allocation));
		}

	private:
		PagedArenaAllocator<T, 512> m_allocator;
	};
}
