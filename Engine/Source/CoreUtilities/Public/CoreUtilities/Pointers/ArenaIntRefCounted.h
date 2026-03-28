#pragma once

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

#include <atomic>

template<typename Type>
class ArenaIntRefCounted
{
public:
	ArenaIntRefCounted(const ArenaIntRefCounted&) noexcept = delete;
	ArenaIntRefCounted& operator=(const ArenaIntRefCounted&) noexcept = delete;
	ArenaIntRefCounted(ArenaIntRefCounted&&) noexcept = delete;
	ArenaIntRefCounted& operator=(ArenaIntRefCounted&&) noexcept = delete;

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

			VT_ENSURE(m_arenaFreeFunc);
			m_arenaFreeFunc(derived, m_arenaPtr);
		}
	}

	VT_INLINE int32_t GetRefCount() const noexcept
	{
		return m_count.load(std::memory_order::relaxed);
	}

	template<typename T>
	VT_INLINE void SetArena(PagedAtomicArenaAllocator<T, 1024>* arena)
	{
		VT_ENSURE_MSG(m_arenaFreeFunc == nullptr, "An arena has already been assigned!");
		if (!m_arenaFreeFunc)
		{
			constexpr auto arenaFreeFunc = [](void* ptr, void* arenaPtr)
			{
				PagedAtomicArenaAllocator<T, 1024>* arena = std::launder(reinterpret_cast<PagedAtomicArenaAllocator<T, 1024>*>(arenaPtr));
				T* valuePtr = std::launder(reinterpret_cast<T*>(ptr));

				arena->Free(valuePtr);
			};

			m_arenaFreeFunc = arenaFreeFunc;
			m_arenaPtr = arena;
		}
	}

protected:
	ArenaIntRefCounted() noexcept
		: m_arenaPtr(nullptr),
		m_arenaFreeFunc(nullptr),
		m_count(1)
	{ }

	virtual ~ArenaIntRefCounted() noexcept
	{
		[[maybe_unused]] auto validCount = [](auto val) { return val == 0 || val == 1; };
		VT_ASSERT(validCount(m_count.load(std::memory_order::relaxed)));
	}

	IntRef<Type> CreateIntRefFromThis() const
	{
		return IntRef<Type>::Attach(const_cast<Type*>(reinterpret_cast<const Type*>(this)));
	}

private:
	typedef void(*ArenaFreeFunc)(void*, void*);

	ArenaFreeFunc m_arenaFreeFunc;
	void* m_arenaPtr;
	mutable std::atomic<int32_t> m_count;
};
