#pragma once

template<typename T>
inline constexpr Weak<T>::Weak()
	: m_ptr(nullptr),
	m_controlBlock(nullptr)
{}

template<typename T>
inline constexpr Weak<T>::~Weak()
{
	Reset();
}

template<typename T>
inline constexpr Weak<T>::Weak(const Weak& other)
{
	CopyInternal(other);
}

template<typename T>
inline constexpr Weak<T>& Weak<T>::operator=(const Weak& other)
{
	if (&other != this)
	{
		CopyInternal(other);
	}

	return *this;
}

template<typename T>
inline constexpr Weak<T>::Weak(Weak&& other) noexcept
{
	MoveInternal(std::move(other));
}

template<typename T>
inline constexpr Weak<T>& Weak<T>::operator=(Weak&& other) noexcept
{
	if (&other != this)
	{
		MoveInternal(std::move(other));
	}

	return *this;
}

template<typename T>
template<typename U>
	requires(std::is_convertible_v<U*, T*>)
inline constexpr Weak<T>::Weak(const Weak<U>& other)
{
	CopyInternal(other);
}

template<typename T>
template<typename U>
	requires(std::is_convertible_v<U*, T*>)
inline constexpr Weak<T>& Weak<T>::operator=(const Weak<U>& other)
{
	CopyInternal(other);
	return *this;
}

template<typename T>
template<typename U>
	requires(std::is_convertible_v<U*, T*>)
inline constexpr Weak<T>::Weak(Weak<U>&& other) noexcept
{
	MoveInternal(std::move(other));
}

template<typename T>
template<typename U>
	requires(std::is_convertible_v<U*, T*>)
inline constexpr Weak<T>& Weak<T>::operator=(Weak<U>&& other) noexcept
{
	MoveInternal(std::move(other));
	return *this;
}

template<typename T>
template<typename U>
	requires(std::is_convertible_v<T*, U*>)
constexpr std::strong_ordering Weak<T>::operator<=>(const Weak<U>& other) const
{
	return m_ptr <=> other.m_ptr;
}

template<typename T>
constexpr std::strong_ordering Weak<T>::operator<=>(std::nullptr_t) const
{
	return m_ptr <=> reinterpret_cast<ElementType*>(nullptr);
}

template<typename T>
inline constexpr Weak<T>::Weak(const Ref<T>& other)
{
	m_ptr = other.m_ptr;
	m_controlBlock = other.m_controlBlock;
	if (m_controlBlock)
	{
		m_controlBlock->IncRefControlBlock();
	}
}

template<typename T>
inline constexpr Weak<T>& Weak<T>::operator=(const Ref<T>& other)
{
	m_ptr = other.m_ptr;
	m_controlBlock = other.m_controlBlock;
	if (m_controlBlock)
	{
		m_controlBlock->IncRefControlBlock();
	}

	return *this;
}

template<typename T>
template<typename U>
inline void Weak<T>::CopyInternal(const Weak<U>& other)
{
	m_ptr = other.m_ptr;
	m_controlBlock = other.m_controlBlock;
	if (m_controlBlock)
	{
		m_controlBlock->IncRefControlBlock();
	}
}

template<typename T>
template<typename U>
inline void Weak<T>::MoveInternal(Weak<U>&& other)
{
	// We inherit the reference count.
	m_ptr = other.m_ptr;
	m_controlBlock = other.m_controlBlock;

	other.m_ptr = nullptr;
	other.m_controlBlock = nullptr;
}

template<typename T>
inline constexpr void Weak<T>::Reset()
{
	m_ptr = nullptr;

	if (m_controlBlock)
	{
		m_controlBlock->DecRefControlBlock();
	}
	m_controlBlock = nullptr;
}

template<typename T>
inline constexpr void Weak<T>::Swap(Weak& other)
{
	std::swap(m_ptr, other.m_ptr);
	std::swap(m_controlBlock, other.m_controlBlock);
}

template<typename T>
inline constexpr Ref<T> Weak<T>::Lock() const
{
	VT_ASSERT(!IsExpired());
	return Ref<T>(m_ptr, m_controlBlock);
}

template<typename T>
inline constexpr bool Weak<T>::IsExpired() const
{
	if (m_controlBlock == nullptr)
	{
		return true;
	}

	return m_controlBlock->GetRefCount() == 0;
}
