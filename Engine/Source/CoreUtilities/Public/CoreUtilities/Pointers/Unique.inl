#pragma once

#include "CoreUtilities/VoltAssert.h"

template<typename T, typename DestroyerType>
constexpr Unique<T, DestroyerType>::Unique()
	: m_ptr(nullptr)
{}

template<typename T, typename DestroyerType>
constexpr Unique<T, DestroyerType>::Unique(T* initalValue)
	: m_ptr(initalValue)
{}

template<typename T, typename DestroyerType>
constexpr Unique<T, DestroyerType>::~Unique()
{
	Reset();
}

template<typename T, typename DestroyerType>
constexpr Unique<T, DestroyerType>::Unique(Unique&& other) noexcept
	: m_ptr(other.m_ptr)
{
	other.m_ptr = nullptr;
}

template<typename T, typename DestroyerType>
constexpr Unique<T, DestroyerType>& Unique<T, DestroyerType>::operator=(Unique&& other) noexcept
{
	if (&other != this)
	{
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
	}

	return *this;
}

template<typename T, typename DestroyerType>
constexpr Unique<T, DestroyerType>::Unique(std::nullptr_t) noexcept
	: m_ptr(nullptr)
{
}

template<typename T, typename DestroyerType>
constexpr Unique<T, DestroyerType>& Unique<T, DestroyerType>::operator=(std::nullptr_t) noexcept
{
	Reset();
	return *this;
}

template<typename T, typename DestroyerType>
template<typename U, typename UDestroyerType>
	requires(std::is_convertible_v<U*, T*>)
constexpr Unique<T, DestroyerType>::Unique(Unique<U, UDestroyerType>&& other) noexcept
	: m_ptr(other.m_ptr)
{
	other.m_ptr = nullptr;
}

template<typename T, typename DestroyerType>
template<typename U, typename UDestroyerType>
	requires(std::is_convertible_v<U*, T*>)
constexpr Unique<T, DestroyerType>& Unique<T, DestroyerType>::operator=(Unique<U, UDestroyerType>&& other) noexcept
{
	m_ptr = other.m_ptr;
	other.m_ptr = nullptr;

	return *this;
}

template<typename T, typename DestroyerType>
constexpr T* Unique<T, DestroyerType>::operator->() noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return m_ptr;
}

template<typename T, typename DestroyerType>
constexpr T& Unique<T, DestroyerType>::operator*() noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return *m_ptr;
}

template<typename T, typename DestroyerType>
constexpr T* Unique<T, DestroyerType>::operator->() const noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return m_ptr;
}

template<typename T, typename DestroyerType>
constexpr T& Unique<T, DestroyerType>::operator*() const noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return *m_ptr;
}

template<typename T, typename DestroyerType>
constexpr bool Unique<T, DestroyerType>::operator==(const Unique& other) const
{
	return m_ptr == other.m_ptr;
}

template<typename T, typename DestroyerType>
constexpr bool Unique<T, DestroyerType>::operator==(std::nullptr_t) const
{
	return m_ptr == nullptr;
}

template<typename T, typename DestroyerType>
constexpr Unique<T, DestroyerType>::operator bool() const
{
	return m_ptr != nullptr;
}

template<typename T, typename DestroyerType>
inline void Unique<T, DestroyerType>::Reset()
{
	if (m_ptr)
	{
		m_destructor(m_ptr);
	}

	m_ptr = nullptr;
}


template<typename T, typename DestroyerType>
void Unique<T, DestroyerType>::Reset(T* initialValue)
{
	Reset();
	m_ptr = initialValue;
}

template<typename T, typename DestroyerType>
inline T* Unique<T, DestroyerType>::GetRaw()
{
	return m_ptr;
}

template<typename T, typename DestroyerType>
T* Unique<T, DestroyerType>::GetRaw() const
{
	return m_ptr;
}
