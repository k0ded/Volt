#pragma once

#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Containers/Array.h"

template<typename T>
class ArrayView
{
public:
	typedef size_t size_type;
	typedef const T* const_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	constexpr ArrayView() noexcept;
	constexpr ArrayView(const ArrayView<T>& other) noexcept;
	constexpr ArrayView(ArrayView<T>&& other) noexcept;

	template<typename Allocator> constexpr ArrayView(const Vector<T, Allocator>& v) noexcept;
	template<size_t N> constexpr ArrayView(const Array<T, N>& arr) noexcept;

	constexpr ArrayView<T>& operator=(const ArrayView<T>& other) noexcept;
	constexpr ArrayView<T>& operator=(ArrayView<T>&& other) noexcept;

	template<typename Allocator> constexpr ArrayView<T>& operator=(const Vector<T, Allocator>& v) noexcept;
	template<size_t N> constexpr ArrayView<T>& operator=(const Array<T, N>& arr) noexcept;

	VT_NODISCARD constexpr const_iterator begin() const noexcept;
	VT_NODISCARD constexpr const_iterator end() const noexcept;
	VT_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept;
	VT_NODISCARD constexpr const_reverse_iterator rend() const noexcept;

	VT_NODISCARD constexpr bool empty() const noexcept;
	VT_NODISCARD constexpr size_type byte_size() const noexcept;
	VT_NODISCARD constexpr size_type size() const noexcept;

	VT_NODISCARD constexpr const T* data() const noexcept;
	
	VT_NODISCARD constexpr const T& operator[](size_type position) const;
	VT_NODISCARD constexpr const T& at(size_type position) const;

	VT_NODISCARD constexpr const T& front() const;
	VT_NODISCARD constexpr const T& back() const;

private:
	const T* m_data;
	size_t m_size;
};

template<typename T>
inline constexpr ArrayView<T>::ArrayView() noexcept
	: m_data(nullptr),
	m_size(0)
{}

template<typename T>
inline constexpr ArrayView<T>::ArrayView(const ArrayView<T>& other) noexcept
	: m_data(other.m_data),
	m_size(other.m_size)
{}

template<typename T>
inline constexpr ArrayView<T>::ArrayView(ArrayView<T> && other) noexcept
	: m_data(other.m_data),
	m_size(other.m_size)
{
	other.m_data = nullptr;
	other.m_size = 0;
}

template<typename T>
inline constexpr ArrayView<T>& ArrayView<T>::operator=(const ArrayView<T>& other) noexcept
{
	if (this != &other)
	{
		m_data = other.m_data;
		m_size = other.m_size;
	}

	return *this;
}

template<typename T>
inline constexpr ArrayView<T>& ArrayView<T>::operator=(ArrayView<T>&& other) noexcept
{
	if (this != &other)
	{
		m_data = other.m_data;
		m_size = other.m_size;

		other.m_data = nullptr;
		other.m_size = 0;
	}

	return *this;
}

template<typename T>
template<typename Allocator> 
inline constexpr ArrayView<T>& ArrayView<T>::operator=(const Vector<T, Allocator>& v) noexcept
{
	m_data = v.data();
	m_size = v.size();

	return *this;
}

template<typename T>
template<typename Allocator>
inline constexpr ArrayView<T>::ArrayView(const Vector<T, Allocator>& v) noexcept
	: m_data(v.data()),
	m_size(v.size())
{}

template<typename T>
template<size_t N>
constexpr ArrayView<T>::ArrayView(const Array<T, N>& arr) noexcept
	: m_data(arr.data()),
	m_size(arr.size())
{
}

template<typename T>
template<size_t N> 
constexpr ArrayView<T>& ArrayView<T>::operator=(const Array<T, N>& arr) noexcept
{
	m_data = arr.data();
	m_size = arr.size();
}

template<typename T>
constexpr ArrayView<T>::const_iterator ArrayView<T>::begin() const noexcept
{
	return m_data;
}

template<typename T>
constexpr ArrayView<T>::const_iterator ArrayView<T>::end() const noexcept
{
	return m_size > 0 ? m_data + m_size : nullptr;
}

template<typename T>
constexpr ArrayView<T>::const_reverse_iterator ArrayView<T>::rbegin() const noexcept
{
	return const_reverse_iterator(end());
}

template<typename T>
constexpr ArrayView<T>::const_reverse_iterator ArrayView<T>::rend() const noexcept
{
	return const_reverse_iterator(begin());
}

template<typename T>
constexpr bool ArrayView<T>::empty() const noexcept
{
	return m_size == 0;
}

template<typename T>
constexpr ArrayView<T>::size_type ArrayView<T>::byte_size() const noexcept
{
	return m_size * sizeof(T);
}

template<typename T>
constexpr ArrayView<T>::size_type ArrayView<T>::size() const noexcept
{
	return m_size;
}

template<typename T>
constexpr const T* ArrayView<T>::data() const noexcept
{
	return m_data;
}

template<typename T>
constexpr const T& ArrayView<T>::operator[](size_type position) const
{
	return m_data[position];
}

template<typename T>
constexpr const T& ArrayView<T>::at(size_type position) const
{
	return m_data[position];
}

template<typename T>
constexpr const T& ArrayView<T>::front() const
{
	return *m_data;
}

template<typename T>
constexpr const T& ArrayView<T>::back() const
{
	return *(m_data + (m_size - 1ull));
}
