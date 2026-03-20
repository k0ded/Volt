#pragma once

template<typename T>
constexpr IntRef<T>::IntRef() noexcept
	: m_object(nullptr)
{}

template<typename T>
constexpr IntRef<T>::~IntRef() noexcept
{
	if (m_object)
	{
		m_object->DecRef();
	}
}

template<typename T>
constexpr IntRef<T>::IntRef(std::nullptr_t) noexcept
	: m_object(nullptr)
{}

template<typename T>
constexpr IntRef<T>& IntRef<T>::operator=(nullptr_t) noexcept
{
	if (m_object)
	{
		m_object->DecRef();
	}

	m_object = nullptr;
	return *this;
}

template<typename T>
constexpr IntRef<T>::IntRef(const IntRef<T>& other) noexcept
	: m_object(other.m_object)
{
	if (m_object)
	{
		m_object->IncRef();
	}
}

template<typename T>
constexpr IntRef<T>& IntRef<T>::operator=(const IntRef<T>& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	T* oldVal = m_object;
	m_object = other.m_object;

	if (m_object)
	{
		m_object->IncRef();
	}

	if (oldVal)
	{
		oldVal->DecRef();
	}

	return *this;
}


template<typename T>
constexpr IntRef<T>::IntRef(IntRef<T>&& other) noexcept
	: m_object(other.m_object)
{
	other.m_object = nullptr;
}

template<typename T>
constexpr IntRef<T>& IntRef<T>::operator=(IntRef<T>&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	if (m_object)
	{
		m_object->DecRef();
	}

	m_object = other.m_object;
	other.m_object = nullptr;

	return *this;
}

template<typename T>
template<typename U>
	requires (std::is_convertible_v<U*, T*>)
constexpr IntRef<T>::IntRef(const IntRef<U>& other) noexcept
	: m_object(static_cast<T*>(other.m_object))
{
	if (m_object)
	{
		m_object->IncRef();
	}
}

template<typename T>
template<typename U>
	requires (std::is_convertible_v<U*, T*>)
constexpr IntRef<T>& IntRef<T>::operator=(const IntRef<U>& other) noexcept
{
	T* newVal = static_cast<T*>(other.m_object);

	if (m_object == newVal)
	{
		return *this;
	}

	T* oldVal = m_object;
	m_object = newVal;

	if (m_object)
	{
		m_object->IncRef();
	}

	if (oldVal)
	{
		oldVal->DecRef();
	}

	return *this;
}

template<typename T>
template<typename U>
	requires (std::is_convertible_v<U*, T*>)
constexpr IntRef<T>::IntRef(IntRef<U>&& other) noexcept
	: m_object(static_cast<T*>(other.m_object))
{
	other.m_object = nullptr;
}

template<typename T>
template<typename U>
	requires (std::is_convertible_v<U*, T*>)
constexpr IntRef<T>& IntRef<T>::operator=(IntRef<U>&& other) noexcept
{
	if (reinterpret_cast<void*>(this) == reinterpret_cast<void*>(&other))
	{
		return *this;
	}

	if (m_object)
	{
		m_object->DecRef();
	}

	m_object = static_cast<T*>(other.m_object);
	other.m_object = nullptr;

	return *this;
}

template<typename T>
constexpr T* IntRef<T>::Release() noexcept
{
	T* ptr = m_object;
	m_object = nullptr;
	return ptr;
}

template<typename T>
constexpr void IntRef<T>::Reset() noexcept
{
	if (m_object)
	{
		m_object->DecRef();
	}

	m_object = nullptr;
}

template<typename T>
constexpr T* IntRef<T>::GetRaw() noexcept
{
	return m_object;
}

template<typename T>
constexpr T* IntRef<T>::GetRaw() const noexcept
{
	return m_object;
}

template<typename T>
constexpr size_t IntRef<T>::GetHash() const
{
	return std::hash<void*>()(m_object);
}

template<typename T>
template<typename U>
constexpr IntRef<U> IntRef<T>::As() const noexcept
{
	return IntRef<U>::Attach(static_cast<U*>(m_object));
}

template<typename T>
constexpr T* IntRef<T>::operator->() noexcept
{
	return m_object;
}

template<typename T>
constexpr T& IntRef<T>::operator*() noexcept
{
	return *m_object;
}

template<typename T>
constexpr T* IntRef<T>::operator->() const noexcept
{
	return m_object;
}

template<typename T>
constexpr T& IntRef<T>::operator*() const noexcept
{
	return *m_object;
}

template<typename T>
constexpr bool IntRef<T>::operator==(const IntRef<T>& rhs) const noexcept
{
	return m_object == rhs.m_object;
}

template<typename T>
constexpr bool IntRef<T>::operator==(std::nullptr_t) const noexcept
{
	return m_object == nullptr;
}

template<typename T>
template<typename U>
constexpr bool IntRef<T>::operator==(const IntRef<U>& rhs) const noexcept
{
	return m_object == rhs.m_object;
}

template<typename T>
template<typename U>
constexpr bool IntRef<T>::operator==(const U* rhs) const noexcept
{
	return m_object == rhs;
}

template<typename T>
template<typename... Args>
static IntRef<T> IntRef<T>::Create(Args&&... args)
{
	void* allocatedPtr = Memory::Malloc(sizeof(T), alignof(T));
	T* objectPtr = new (allocatedPtr) T(std::forward<Args>(args)...);
	return IntRef<T>(objectPtr);
}

template<typename T>
IntRef<T> IntRef<T>::Attach(T* ptr)
{
	IntRef<T> refPtr;
	refPtr.m_object = ptr;
	if (ptr)
	{
		ptr->IncRef();
	}
	return refPtr;
}

template<typename T>
IntRef<T> IntRef<T>::AttachNoRef(T* ptr)
{
	IntRef<T> refPtr;
	refPtr.m_object = ptr;
	return refPtr;
}
