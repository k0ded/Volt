#pragma once

#include "CoreUtilities/Core.h"
#include "CoreUtilities/Pointers/IntRef.h"
#include "CoreUtilities/Malloc.h"

#include <atomic>

template<typename Type>
class IntRefCounted
{
public:
	IntRefCounted(const IntRefCounted&) noexcept = delete;
	IntRefCounted& operator=(const IntRefCounted&) noexcept = delete;
	IntRefCounted(IntRefCounted&&) noexcept = delete;
	IntRefCounted& operator=(IntRefCounted&&) noexcept = delete;

	VT_INLINE void IncRef() const noexcept
	{
		[[maybe_unused]] auto oldValue = m_count.fetch_add(1, std::memory_order::relaxed);
		VT_ASSERT(oldValue > 0);
	}

	VT_INLINE void DecRef() const noexcept
	{
		auto oldCount = m_count.fetch_sub(1, std::memory_order::release);
		VT_ASSERT(oldCount > 0);

		if (oldCount == 1)
		{
			std::atomic_thread_fence(std::memory_order::acquire);

			Type* derived = const_cast<Type*>(static_cast<const Type*>(this));
			derived->~Type();

			Memory::Free(derived);
		}
	}

	VT_INLINE int32_t GetRefCount() const noexcept
	{
		return m_count.load(std::memory_order::relaxed);
	}

protected:
	IntRefCounted() noexcept = default;
	virtual ~IntRefCounted() noexcept
	{
		[[maybe_unused]] auto validCount = [](auto val) { return val == 0 || val == 1; };
		VT_ASSERT(validCount(m_count.load(std::memory_order::relaxed)));
	}

	IntRef<Type> CreateIntRefFromThis() const
	{
		return IntRef<Type>::Attach(const_cast<Type*>(reinterpret_cast<const Type*>(this)));
	}

private:
	mutable std::atomic<int32_t> m_count = 1;
};
