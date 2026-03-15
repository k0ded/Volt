#pragma once

#include "AssetSystem/Asset.h"
#include "AssetSystem/AssetManagerCommon.h"

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

namespace Volt
{
	class AssetTypeAllocator
	{
	public:
		virtual ~AssetTypeAllocator() = default;
		virtual Asset* AllocateDefault() = 0;
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

		Asset* AllocateDefault() override
		{
			return m_allocator.Allocate();
		}

		void Free(void* allocation) override
		{
			m_allocator.Free(reinterpret_cast<T*>(allocation));
		}

	private:
		PagedAtomicArenaAllocator<T, 512> m_allocator;
	};
}
