#pragma once

#include "CoreUtilities/Containers/Vector.h"

#include <mutex>
#include <shared_mutex>

template<typename T, typename AllocatorType = DefaultHeapAllocator>
class ThreadSafeVector : protected Vector<T, AllocatorType>
{
public:
	using WriteLock = std::unique_lock<std::shared_mutex>;
	using ReadLock = std::shared_lock<std::shared_mutex>;

	VT_INLINE ThreadSafeVector() noexcept = default;

	VT_INLINE ThreadSafeVector(Vector<T, AllocatorType>::size_type count) noexcept
		: Vector<T, AllocatorType>(count)
	{}

	VT_INLINE ThreadSafeVector(Vector<T, AllocatorType>::size_type count, const T& value) noexcept
		: Vector<T, AllocatorType>(count, value)
	{ }

	VT_INLINE ThreadSafeVector(const ThreadSafeVector& other) noexcept
		: Vector<T, AllocatorType>(other)
	{};
	
	VT_INLINE ThreadSafeVector(ThreadSafeVector&& other) noexcept
		: Vector<T, AllocatorType>(std::move(other))
	{}

	VT_INLINE ~ThreadSafeVector() = default;


	VT_INLINE ThreadSafeVector& operator=(const ThreadSafeVector& other)
	{
		Vector<T, AllocatorType>::operator=(other);
		return *this;
	}

	VT_INLINE ThreadSafeVector& operator=(ThreadSafeVector&& other)
	{
		Vector<T, AllocatorType>::operator=(std::move(other));
		return *this;
	}

	VT_INLINE constexpr void clear()
	{
		WriteLock lock{ m_mutex };
		return Vector<T, AllocatorType>::clear();
	}

	VT_NODISCARD VT_INLINE constexpr bool empty() const
	{
		ReadLock lock{ m_mutex };
		return Vector<T, AllocatorType>::empty();
	}

	VT_NODISCARD VT_INLINE constexpr T& back()
	{
		ReadLock lock{ m_mutex };
		return Vector<T, AllocatorType>::back();
	}

	VT_NODISCARD VT_INLINE constexpr const T& back() const
	{
		ReadLock lock{ m_mutex };
		return Vector<T, AllocatorType>::back();
	}

	VT_INLINE constexpr void pop_back()
	{
		WriteLock lock{ m_mutex };
		return Vector<T, AllocatorType>::pop_back();
	}

	VT_INLINE constexpr void push_back(const T& value)
	{
		WriteLock lock{ m_mutex };
		Vector<T, AllocatorType>::push_back(value);
	}

	VT_NODISCARD VT_INLINE constexpr T& push_back()
	{ 
		WriteLock lock{ m_mutex };
		return Vector<T, AllocatorType>::push_back();
	}
	
	VT_INLINE constexpr void push_back(T&& value)
	{
		WriteLock lock{ m_mutex };
		Vector<T, AllocatorType>::push_back(std::move(value));
	}

	template<typename... Args>
	VT_INLINE constexpr T& emplace_back(Args&&... args)
	{
		WriteLock lock{ m_mutex };
		return Vector<T, AllocatorType>::emplace_back(std::forward<Args>(args)...);
	}

	VT_NODISCARD VT_INLINE T& operator[](Vector<T, AllocatorType>::size_type index)
	{
		ReadLock lock{ m_mutex };
		return Vector<T, AllocatorType>::operator[](index);
	}

	VT_NODISCARD VT_INLINE const T& operator[](Vector<T, AllocatorType>::size_type index) const
	{
		ReadLock lock{ m_mutex };
		return Vector<T, AllocatorType>::operator[](index);
	}

	VT_NODISCARD VT_INLINE Vector<T, AllocatorType>::iterator begin()
	{
		WriteLock lock{ m_mutex };
		return Vector<T, AllocatorType>::begin();
	}

	VT_NODISCARD VT_INLINE Vector<T, AllocatorType>::iterator end()
	{
		WriteLock lock{ m_mutex };
		return Vector<T, AllocatorType>::end();
	}

	VT_NODISCARD VT_INLINE Vector<T, AllocatorType>::const_iterator begin() const
	{
		WriteLock lock{ m_mutex };
		return Vector<T, AllocatorType>::begin();
	}

	VT_NODISCARD VT_INLINE Vector<T, AllocatorType>::const_iterator end() const
	{
		WriteLock lock{ m_mutex };
		return Vector<T, AllocatorType>::end();
	}

private:
	mutable std::shared_mutex m_mutex;
};
