#pragma once

#include "CoreUtilities/VoltAssert.h"

template<typename T, size_t N = 1>
struct Array
{
public:
	typedef Array<T, N> this_type;
	typedef T value_type;
	typedef size_t size_type;
	typedef T* iterator;
	typedef const T* const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	// Intentionally public to allow for aggregate initialization:
	// Array<int, 2> arr = { 1, 2 };
	value_type m_data[N];

	void fill(const value_type& value);
	void swap(this_type& other) noexcept;

	constexpr iterator begin() noexcept;
	constexpr const_iterator begin() const noexcept;
	constexpr const_iterator cbegin() const noexcept;

	constexpr iterator end() noexcept;
	constexpr const_iterator end() const noexcept;
	constexpr const_iterator cend() const noexcept;

	constexpr reverse_iterator rbegin() noexcept;
	constexpr const_reverse_iterator rbegin() const noexcept;
	constexpr const_reverse_iterator crbegin() const noexcept;

	constexpr reverse_iterator rend() noexcept;
	constexpr const_reverse_iterator rend() const noexcept;
	constexpr const_reverse_iterator crend() const noexcept;

	constexpr bool empty() const noexcept;
	constexpr size_type size() const noexcept;
	constexpr size_type byte_size() const noexcept;
	constexpr size_type max_size() const noexcept;

	constexpr T* data() noexcept;
	constexpr const T* data() const noexcept;

	constexpr T& operator[](size_type i);
	constexpr const T& operator[](size_type i) const;
	constexpr const T& at(size_type i) const;
	constexpr T& at(size_type i);

	constexpr T& front();
	constexpr const T& front() const;

	constexpr T& back();
	constexpr const T& back() const;

	bool validate() const;
	bool validate_iterator(const_iterator it) const;
};

// Template deduction guide
template<class T, class... U> Array(T, U...) -> Array<T, 1 + sizeof...(U)>;

template<typename T, size_t N>
inline void Array<T, N>::fill(const value_type& value)
{
	for (size_type i = 0; i < N; ++i)
	{
		m_data[i] = value;
	}
}

template<typename T, size_t N>
inline void Array<T, N>::swap(this_type& other) noexcept
{
	for (size_type i = 0; i < N; ++i)
	{
		std::swap(m_data[i], other.m_data[i]);
	}
}

template<typename T, size_t N>
inline constexpr Array<T, N>::iterator Array<T, N>::begin() noexcept
{
	return &m_data[0];
}

template<typename T, size_t N>
inline constexpr Array<T, N>::const_iterator Array<T, N>::begin() const noexcept
{
	return &m_data[0];
}

template<typename T, size_t N>
inline constexpr Array<T, N>::const_iterator Array<T, N>::cbegin() const noexcept
{
	return &m_data[0];
}

template<typename T, size_t N>
inline constexpr Array<T, N>::iterator Array<T, N>::end() noexcept
{
	return &m_data[N];
}

template<typename T, size_t N>
inline constexpr Array<T, N>::const_iterator Array<T, N>::end() const noexcept
{
	return &m_data[N];
}

template<typename T, size_t N>
inline constexpr Array<T, N>::const_iterator Array<T, N>::cend() const noexcept
{
	return &m_data[N];
}

template<typename T, size_t N>
inline constexpr Array<T, N>::reverse_iterator Array<T, N>::rbegin() noexcept
{
	return reverse_iterator(&m_data[N]);
}

template<typename T, size_t N>
inline constexpr Array<T, N>::const_reverse_iterator Array<T, N>::rbegin() const noexcept
{
	return const_reverse_iterator(&m_data[N]);
}

template<typename T, size_t N>
inline constexpr Array<T, N>::const_reverse_iterator Array<T, N>::crbegin() const noexcept
{
	return const_reverse_iterator(&m_data[N]);
}

template<typename T, size_t N>
inline constexpr Array<T, N>::reverse_iterator Array<T, N>::rend() noexcept
{
	return reverse_iterator(&m_data[0]);
}

template<typename T, size_t N>
inline constexpr Array<T, N>::const_reverse_iterator Array<T, N>::rend() const noexcept
{
	return const_reverse_iterator(&m_data[0]);
}

template<typename T, size_t N>
inline constexpr Array<T, N>::const_reverse_iterator Array<T, N>::crend() const noexcept
{
	return const_reverse_iterator(&m_data[0]);
}

template<typename T, size_t N>
inline constexpr bool Array<T, N>::empty() const noexcept
{
	return (N == 0);
}

template<typename T, size_t N>
inline constexpr Array<T, N>::size_type Array<T, N>::size() const noexcept
{
	return static_cast<size_type>(N);
}

template<typename T, size_t N /*= 1*/>
inline constexpr Array<T, N>::size_type Array<T, N>::byte_size() const noexcept
{
	return static_cast<size_type>(N) * sizeof(T);
}

template<typename T, size_t N>
inline constexpr Array<T, N>::size_type Array<T, N>::max_size() const noexcept
{
	return static_cast<size_type>(N);
}

template<typename T, size_t N>
inline constexpr T* Array<T, N>::data() noexcept
{
	return m_data;
}

template<typename T, size_t N>
inline constexpr const T* Array<T, N>::data() const noexcept
{
	return m_data;
}

template<typename T, size_t N>
inline constexpr T& Array<T, N>::operator[](size_type i)
{
	VT_ASSERT_MSG(i >= 0 && i < N, "Array::operator[] - Out of range!");
	return m_data[i];
}

template<typename T, size_t N>
inline constexpr const T& Array<T, N>::operator[](size_type i) const
{
	VT_ASSERT_MSG(i >= 0 && i < N, "Array::operator[] - Out of range!");
	return m_data[i];
}

template<typename T, size_t N>
inline constexpr const T& Array<T, N>::at(size_type i) const
{
	VT_ASSERT_MSG(i >= 0 && i < N, "Array::operator[] - Out of range!");
	return m_data[i];
}

template<typename T, size_t N>
inline constexpr T& Array<T, N>::at(size_type i)
{
	VT_ASSERT_MSG(i >= 0 && i < N, "Array::operator[] - Out of range!");
	return m_data[i];
}

template<typename T, size_t N>
inline constexpr T& Array<T, N>::front()
{
	return m_data[0];
}

template<typename T, size_t N>
inline constexpr const T& Array<T, N>::front() const
{
	return m_data[0];
}

template<typename T, size_t N>
inline constexpr T& Array<T, N>::back()
{
	return m_data[N - 1];
}

template<typename T, size_t N>
inline constexpr const T& Array<T, N>::back() const
{
	return m_data[N - 1];
}

template<typename T, size_t N>
inline bool Array<T, N>::validate() const
{
	return true;
}

template<typename T, size_t N>
inline bool Array<T, N>::validate_iterator(const_iterator it) const
{
	return (it < (m_data + N) && it >= m_data);
}

template<typename T, size_t N>
constexpr inline bool operator==(const Array<T, N>& a, const Array<T, N>& b)
{
	bool allEqual = true;
	for (size_t i = 0; i < N; ++i)
	{
		allEqual &= (a[i] == b[i]);
	}

	return allEqual;
}

template<typename T, size_t N>
constexpr inline bool operator!=(const Array<T, N>& a, const Array<T, N>& b)
{
	bool allEqual = true;
	for (size_t i = 0; i < N; ++i)
	{
		allEqual &= (a[i] == b[i]);
	}

	return !allEqual;
}

// Structured bindings
namespace Internal
{
	template<size_t I, typename T, size_t N, typename = void>
	struct tuple_element {};

	template<size_t I, typename T, size_t N>
	struct tuple_element<I, T, N, std::enable_if_t<(I < N)>>
	{
		using type = T;
	};
}

namespace std
{
	template<typename T, size_t N>
	struct tuple_size<Array<T, N>> : public integral_constant<size_t, N> {};

	template<size_t I, typename T, size_t N>
	struct tuple_element<I, Array<T, N>> : public Internal::tuple_element<I, T, N> {};
}
