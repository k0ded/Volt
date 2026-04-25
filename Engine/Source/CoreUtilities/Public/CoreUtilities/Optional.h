#pragma once

#include "CoreUtilities/VoltAssert.h"

template<typename T>
class Optional
{
public:
	constexpr Optional() noexcept = default;
	constexpr ~Optional() noexcept = default;
	 
	constexpr Optional(const T& value) noexcept;
	constexpr Optional(T&& value) noexcept;

	constexpr Optional(const Optional& value) noexcept;
	constexpr Optional(Optional&& value) noexcept;

	constexpr Optional& operator=(const T& value) noexcept;
	constexpr Optional& operator=(T&& value) noexcept;
	constexpr Optional& operator=(const Optional& other) noexcept;
	constexpr Optional& operator=(Optional&& other) noexcept;

	constexpr T& Get();
	constexpr const T& Get() const;

	constexpr bool HasValue() const;

private:
	T m_value;
	bool m_hasValue = false;
};

template<typename T>
inline constexpr Optional<T>::Optional(const T& value) noexcept
	: m_value(value),
	m_hasValue(true)
{}

template<typename T>
inline constexpr Optional<T>::Optional(T&& value) noexcept
	: m_value(std::move(value)),
	m_hasValue(true)
{}

template<typename T>
inline constexpr Optional<T>::Optional(const Optional& other) noexcept
	: m_value(other.Get()),
	m_hasValue(other.m_hasValue)
{
}

template<typename T>
inline constexpr Optional<T>::Optional(Optional&& other) noexcept
	: m_value(std::move(other.m_value)),
	m_hasValue(other.m_hasValue)
{
}

template<typename T>
inline constexpr Optional<T>& Optional<T>::operator=(const T& value) noexcept
{
	m_value = value;
	m_hasValue = true;
	return *this;
}

template<typename T>
inline constexpr Optional<T>& Optional<T>::operator=(T&& value) noexcept
{
	m_value = std::move(value);
	m_hasValue = true;

	return *this;
}

template<typename T>
inline constexpr Optional<T>& Optional<T>::operator=(const Optional& other) noexcept
{
	if (&other != this)
	{
		m_value = other.m_value;
		m_hasValue = other.m_hasValue;
	}

	return *this;
}

template<typename T>
inline constexpr Optional<T>& Optional<T>::operator=(Optional&& other) noexcept
{
	if (&other != this)
	{
		m_value = std::move(other.m_value);
		m_hasValue = other.m_hasValue;
	}

	return *this;
}

template<typename T>
inline constexpr T& Optional<T>::Get()
{
	VT_ASSERT(m_hasValue);
	return m_value;
}

template<typename T>
inline constexpr const T& Optional<T>::Get() const
{
	VT_ASSERT(m_hasValue);
	return m_value;
}

template<typename T>
inline constexpr bool Optional<T>::HasValue() const
{
	return m_hasValue;
}
