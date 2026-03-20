#pragma once

#include "IntRef.h"

template<typename T>
class RawPtr
{
public:
	constexpr RawPtr() noexcept = default;

	constexpr RawPtr(T* ptr) noexcept
		: m_object(ptr)
	{}

	constexpr RawPtr(const IntRef<T>& refPtr) noexcept
		: m_object(refPtr.GetRaw())
	{}

	template<typename U>
	constexpr RawPtr(const IntRef<U>& refPtr) noexcept
		requires (std::is_convertible_v<U*, T*>)
	: m_object(static_cast<T*>(refPtr.GetRaw()))
	{}

	constexpr RawPtr(const RawPtr<T>& other) noexcept
		: m_object(other.GetRaw())
	{}

	constexpr RawPtr(RawPtr<T>&& other) noexcept
		: m_object(other.GetRaw())
	{}

	template<typename U>
	constexpr RawPtr(RawPtr<U>&& other) noexcept
		requires (std::is_convertible_v<U*, T*>)
	: m_object(static_cast<T*>(other.GetRaw()))
	{}

	template<typename U>
	constexpr RawPtr(const RawPtr<U>& other) noexcept
		requires (std::is_convertible_v<U*, T*>)
	: m_object(static_cast<T*>(other.GetRaw()))
	{}

	~RawPtr() noexcept
	{
		m_object = nullptr;
	}

	VT_NODISCARD constexpr T* GetRaw() noexcept { return m_object; }
	VT_NODISCARD constexpr T* GetRaw() const noexcept { return m_object; }

	VT_NODISCARD constexpr std::size_t GetHash() const
	{
		return std::hash<void*>()(m_object);
	}

	constexpr void Reset() noexcept { m_object = nullptr; }

	VT_NODISCARD constexpr bool IsValid() const { return m_object != nullptr; }

	// Note: Not type safe!
	template<typename U>
	VT_NODISCARD constexpr RawPtr<U> As() const noexcept
	{
		return RawPtr<U>(static_cast<U*>(m_object));
	}

	VT_NODISCARD constexpr T* operator->() noexcept { return m_object; }
	VT_NODISCARD constexpr T& operator*() noexcept { return *m_object; }

	VT_NODISCARD constexpr T* operator->() const noexcept { return m_object; }
	VT_NODISCARD constexpr T& operator*() const noexcept { return *m_object; }

	constexpr RawPtr<T>& operator=(const RawPtr<T>& other) noexcept
	{
		m_object = other.GetRaw();
		return *this;
	}

	VT_INLINE RawPtr<T>& operator=(RawPtr<T>&& other) noexcept
	{
		m_object = other.GetRaw();
		return *this;
	}

	template<typename U>
	VT_INLINE RawPtr<T>& operator=(RawPtr<U>&& other) noexcept
		requires (std::is_convertible_v<U*, T*>)
	{
		m_object = static_cast<T*>(other.GetRaw());
		return *this;
	}

	template<typename U>
	VT_INLINE RawPtr<T>& operator=(const RawPtr<U>& other) noexcept
		requires (std::is_convertible_v<U*, T*>)
	{
		m_object = static_cast<T*>(other.GetRaw());
		return *this;
	}

	VT_NODISCARD VT_INLINE bool operator==(const RawPtr<T>& rhs) const noexcept
	{
		return m_object == rhs.m_object;
	}

	VT_NODISCARD VT_INLINE operator bool() const { return m_object != nullptr; }

private:
	T* m_object = nullptr;
};

namespace std
{
	template<typename T>
	struct hash<RawPtr<T>>
	{
		std::size_t operator()(const RawPtr<T>& ptr) const noexcept
		{
			return ptr.GetHash();
		}
	};
}
