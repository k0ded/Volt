#pragma once

#include "RefPtr.h"

template<typename T>
class RawPtr
{
public:
	constexpr RawPtr() noexcept = default;
	
	constexpr RawPtr(T* ptr) noexcept
		: m_object(ptr)
	{}

	constexpr RawPtr(const RefPtr<T>& refPtr) noexcept
		: m_object(refPtr.GetRaw())
	{}

	template<typename U>
	constexpr RawPtr(const RefPtr<U>& refPtr) noexcept requires (std::is_base_of<T, U>::value || std::is_base_of<U, T>::value)
		: m_object(refPtr.GetRaw())
	{}

	constexpr RawPtr(const RawPtr<T>& weakPtr) noexcept
		: m_object(weakPtr.GetRaw())
	{
	}

	constexpr RawPtr(RawPtr<T>&& other) noexcept
		: m_object(other.GetRaw())
	{}

	template<typename U>
	constexpr RawPtr(RawPtr<U>&& other) noexcept requires (std::is_base_of<T, U>::value || std::is_base_of<U, T>::value)
		: m_object(other.GetRaw())
	{
	}

	template<typename U>
	RawPtr(const RawPtr<U>& other) noexcept requires (std::is_base_of<T, U>::value || std::is_base_of<U, T>::value)
		: m_object(reinterpret_cast<T*>(other.GetRaw()))
	{}

	~RawPtr() noexcept
	{
		m_object = nullptr;
	}

	VT_NODISCARD constexpr T* GetRaw() noexcept
	{
		return m_object;
	}

	VT_NODISCARD constexpr T* GetRaw() const noexcept
	{
		return m_object;
	}

	VT_NODISCARD constexpr const size_t GetHash() const
	{
		return std::hash<void*>()(m_object);
	}

	constexpr void Reset() noexcept
	{
		m_object = nullptr;
	}

	VT_NODISCARD constexpr bool IsValid() const
	{
		return m_object != nullptr;
	}

	template<typename U>
	VT_NODISCARD constexpr RawPtr<U> As() const noexcept requires (std::is_base_of<T, U>::value || std::is_base_of<U, T>::value)
	{
		return RawPtr<U>(const_cast<RawPtr<T>&>(*this));
	}

	VT_NODISCARD constexpr T* operator->() noexcept
	{
		return m_object;
	}

	VT_NODISCARD constexpr T& operator*() noexcept
	{
		return *m_object;
	}

	VT_NODISCARD constexpr T* operator->() const noexcept
	{
		return m_object;
	}

	VT_NODISCARD constexpr T& operator*() const noexcept
	{
		return *m_object;
	}

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
	VT_INLINE RawPtr<T>& operator=(RawPtr<U>&& other) noexcept requires (std::is_base_of<T, U>::value || std::is_base_of<U, T>::value)
	{
		m_object = other.GetRaw();
		return *this;
	}

	template<typename U>
	VT_INLINE RawPtr<T>& operator=(const RawPtr<U>& other) noexcept requires (std::is_base_of<T, U>::value || std::is_base_of<U, T>::value)
	{
		m_object = other.GetRaw();
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
	template<typename T> struct hash;

	template<class Ty>
	struct hash<RawPtr<Ty>>
	{
		std::size_t operator()(const RawPtr<Ty> ptr) const
		{
			return ptr.GetHash();
		}
	};
}
