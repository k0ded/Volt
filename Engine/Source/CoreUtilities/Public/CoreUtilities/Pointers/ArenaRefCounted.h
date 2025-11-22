#pragma once

#include <CoreUtilities/Allocators/FixedSizeArenaAllocator.h>

#include <atomic>

template<typename Type>
class ArenaRefCounted
{
public:
	ArenaRefCounted(const ArenaRefCounted&) noexcept = delete;
	ArenaRefCounted& operator=(const ArenaRefCounted&) noexcept = delete;
	ArenaRefCounted(ArenaRefCounted&&) noexcept = delete;
	ArenaRefCounted& operator=(ArenaRefCounted&&) noexcept = delete;

	VT_INLINE void IncRef() const noexcept
	{
		[[maybe_unused]] auto oldValue = m_count.fetch_add(1, std::memory_order_relaxed);
		VT_ASSERT(oldValue > 0);
	}

	VT_INLINE void DecRef() const noexcept
	{
		auto oldCount = m_count.fetch_sub(1, std::memory_order_release);
		VT_ASSERT(oldCount > 0);

		if (oldCount == 1)
		{
			std::atomic_thread_fence(std::memory_order_acquire);

			Type* derived = const_cast<Type*>(static_cast<const Type*>(this));

			VT_ENSURE(m_arenaFreeFunc);
			m_arenaFreeFunc(derived, m_arenaPtr);
		}
	}

	VT_INLINE int32_t GetRefCount() const noexcept
	{
		return m_count.load(std::memory_order_relaxed);
	}

	template<typename T>
	VT_INLINE void SetArena(FixedSizeArenaAllocator<T>* arena)
	{
		VT_ENSURE_MSG(m_arenaFreeFunc == nullptr, "An arena has already been assigned!");
		if (!m_arenaFreeFunc)
		{
			constexpr auto arenaFreeFunc = [](void* ptr, void* arenaPtr)
			{
				reinterpret_cast<FixedSizeArenaAllocator<T>*>(arenaPtr)->Free(reinterpret_cast<T*>(ptr));
			};

			m_arenaFreeFunc = arenaFreeFunc;
			m_arenaPtr = arena;
		}
	}

protected:
	ArenaRefCounted() noexcept
		: m_arenaPtr(nullptr),
		m_arenaFreeFunc(nullptr),
		m_count(1)
	{ }

	virtual ~ArenaRefCounted() noexcept
	{
		[[maybe_unused]] auto validCount = [](auto val) { return val == 0 || val == 1; };
		VT_ASSERT(validCount(m_count.load(std::memory_order_relaxed)));
	}

	RefPtr<Type> CreateRefPtrFromThis() const
	{
		return RefPtr<Type>::Attach(const_cast<Type*>(reinterpret_cast<const Type*>(this)));
	}

private:
	typedef void(*ArenaFreeFunc)(void*, void*);

	ArenaFreeFunc m_arenaFreeFunc;
	void* m_arenaPtr;
	mutable std::atomic<int32_t> m_count;
};
