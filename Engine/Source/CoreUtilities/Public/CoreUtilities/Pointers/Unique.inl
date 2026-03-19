#pragma once

#include "CoreUtilities/VoltAssert.h"

template<typename T, typename DestructorType>
constexpr Unique<T, DestructorType>::Unique()
	: m_ptr(nullptr)
{}

template<typename T, typename DestructorType>
constexpr Unique<T, DestructorType>::Unique(T* initalValue)
	: m_ptr(initalValue)
{}

template<typename T, typename DestructorType>
constexpr Unique<T, DestructorType>::~Unique()
{
	Reset();
}

template<typename T, typename DestructorType>
constexpr Unique<T, DestructorType>::Unique(Unique&& other) noexcept
	: m_ptr(other.m_ptr)
{
	other.m_ptr = nullptr;
}

template<typename T, typename DestructorType>
constexpr Unique<T, DestructorType>& Unique<T, DestructorType>::operator=(Unique&& other) noexcept
{
	if (&other != this)
	{
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
	}

	return *this;
}

template<typename T, typename DestructorType>
constexpr Unique<T, DestructorType>::Unique(std::nullptr_t) noexcept
	: m_ptr(nullptr)
{
}

template<typename T, typename DestructorType>
constexpr Unique<T, DestructorType>& Unique<T, DestructorType>::operator=(std::nullptr_t) noexcept
{
	Reset();
	return *this;
}

template<typename T, typename DestructorType>
template<typename U, typename UDestructorType>
	requires(std::is_convertible_v<U*, T*>)
constexpr Unique<T, DestructorType>::Unique(Unique<U, UDestructorType>&& other) noexcept
	: m_ptr(other.m_ptr)
{
	other.m_ptr = nullptr;
}

template<typename T, typename DestructorType>
template<typename U, typename UDestructorType>
	requires(std::is_convertible_v<U*, T*>)
constexpr Unique<T, DestructorType>& Unique<T, DestructorType>::operator=(Unique<U, UDestructorType>&& other) noexcept
{
	m_ptr = other.m_ptr;
	other.m_ptr = nullptr;

	return *this;
}

template<typename T, typename DestructorType>
constexpr T* Unique<T, DestructorType>::operator->() noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return m_ptr;
}

template<typename T, typename DestructorType>
constexpr T& Unique<T, DestructorType>::operator*() noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return *m_ptr;
}

template<typename T, typename DestructorType>
constexpr T* Unique<T, DestructorType>::operator->() const noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return m_ptr;
}

template<typename T, typename DestructorType>
constexpr T& Unique<T, DestructorType>::operator*() const noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return *m_ptr;
}

template<typename T, typename DestructorType>
constexpr bool Unique<T, DestructorType>::operator==(const Unique& other) const
{
	return m_ptr == other.m_ptr;
}

template<typename T, typename DestructorType>
constexpr Unique<T, DestructorType>::operator bool() const
{
	return m_ptr != nullptr;
}

template<typename T, typename DestructorType>
inline void Unique<T, DestructorType>::Reset()
{
	if (m_ptr)
	{
		m_destructor(m_ptr);
	}

	m_ptr = nullptr;
}


template<typename T, typename DestructorType>
void Unique<T, DestructorType>::Reset(T* initialValue)
{
	Reset();
	m_ptr = initialValue;
}

template<typename T, typename DestructorType>
inline T* Unique<T, DestructorType>::GetRaw()
{
	return m_ptr;
}

template<typename T, typename DestructorType>
T* Unique<T, DestructorType>::GetRaw() const
{
	return m_ptr;
}
