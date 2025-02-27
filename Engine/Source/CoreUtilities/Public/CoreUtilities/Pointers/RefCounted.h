#pragma once

#include "CoreUtilities/Core.h"
#include "CoreUtilities/Pointers/RefPtr.h"

#include "CoreUtilities/Allocators/DefaultAllocator.h"

#include <atomic>

template<typename Type, class AllocatorType = DefaultAllocator>
class RefCounted
{
public:
	using Allocator = AllocatorType;

	RefCounted(const RefCounted&) noexcept = delete;
	RefCounted& operator=(const RefCounted&) noexcept = delete;
	RefCounted(RefCounted&&) noexcept = delete;
	RefCounted& operator=(RefCounted&&) noexcept = delete;

	void IncRef() const noexcept
	{
		[[maybe_unused]] auto oldValue = m_count.fetch_add(1, std::memory_order_relaxed);
		VT_ASSERT(oldValue > 0);
	}

	void DecRef() const noexcept
	{
		auto oldCount = m_count.fetch_sub(1, std::memory_order_release);
		VT_ASSERT(oldCount > 0);

		if (oldCount == 1)
		{
			std::atomic_thread_fence(std::memory_order_acquire);

			Type* derived = const_cast<Type*>(static_cast<const Type*>(this));
			derived->~Type();
			Allocator::Free(derived, alignof(Type));
		}
	}

protected:
	RefCounted() noexcept = default;
	virtual ~RefCounted() noexcept
	{
		[[maybe_unused]] auto validCount = [](auto val) { return val == 0 || val == 1; };
		VT_ASSERT(validCount(m_count.load(std::memory_order_relaxed)));
	}

	RefPtr<Type> CreateRefPtrFromThis() const
	{
		return RefPtr<Type>::Attach(const_cast<Type*>(reinterpret_cast<const Type*>(this)));
	}

private:
	mutable std::atomic<int32_t> m_count = 1;
};
