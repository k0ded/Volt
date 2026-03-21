#pragma once

template<typename T>
inline constexpr Ref<T>::Ref()
	: m_ptr(nullptr),
	m_controlBlock(nullptr)
{}

template<typename T>
inline constexpr Ref<T>::~Ref()
{
	Reset();
}

template<typename T>
inline constexpr Ref<T>::Ref(const Ref& other)
{
	CopyInternal(other);
}

template<typename T>
inline constexpr Ref<T>& Ref<T>::operator=(const Ref& other)
{
	if (&other != this)
	{
		CopyInternal(other);
	}

	return *this;
}

template<typename T>
inline constexpr Ref<T>::Ref(Ref&& other) noexcept
{
	MoveInternal(std::move(other));
}

template<typename T>
inline constexpr Ref<T>& Ref<T>::operator=(Ref&& other) noexcept
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
inline constexpr Ref<T>::Ref(const Ref<U>& other)
{
	CopyInternal(other);
}

template<typename T>
template<typename U>
	requires(std::is_convertible_v<U*, T*>)
inline constexpr Ref<T>& Ref<T>::operator=(const Ref<U>& other)
{
	CopyInternal(other);
	return *this;
}

template<typename T>
template<typename U>
	requires(std::is_convertible_v<U*, T*>)
inline constexpr Ref<T>::Ref(Ref<U>&& other) noexcept
{
	MoveInternal(std::move(other));
}

template<typename T>
template<typename U>
	requires(std::is_convertible_v<U*, T*>)
inline constexpr Ref<T>& Ref<T>::operator=(Ref<U>&& other) noexcept
{
	MoveInternal(std::move(other));
	return *this;
}

template<typename T>
template<typename U>
constexpr Ref<T>::Ref(Ref<U>&& other, T* initialPtr) noexcept
{
	m_ptr = initialPtr;
	m_controlBlock = other.m_controlBlock;

	other.m_controlBlock = nullptr;
	other.m_ptr = nullptr;
}

template<typename T>
template<typename U>
constexpr Ref<T>::Ref(const Ref<U>& other, T* initialPtr)
{
	m_ptr = initialPtr;
	m_controlBlock = other.m_controlBlock;

	if (m_controlBlock)
	{
		m_controlBlock->IncRef();
		m_controlBlock->IncRefControlBlock();
	}
}

template<typename T>
inline constexpr Ref<T>::Ref(ElementType* initialValue)
{
	Reset(initialValue);
}

template<typename T>
template<typename Destroyer>
constexpr Ref<T>::Ref(ElementType* initialValue, Destroyer&& destructor)
{
	ResetInternal(initialValue, std::move(destructor));
}

template<typename T>
inline constexpr Ref<T>::Ref(std::nullptr_t) noexcept
{
	Reset();
}

template<typename T>
inline constexpr Ref<T>& Ref<T>::operator=(std::nullptr_t) noexcept
{
	Reset();
	return *this;
}

template<typename T>
inline constexpr T* Ref<T>::operator->() noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return m_ptr;
}

template<typename T>
inline constexpr T& Ref<T>::operator*() noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return *m_ptr;
}

template<typename T>
inline constexpr T* Ref<T>::operator->() const noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return m_ptr;
}

template<typename T>
inline constexpr T& Ref<T>::operator*() const noexcept
{
	VT_ASSERT(m_ptr != nullptr);
	return *m_ptr;
}

template<typename T>
template<typename U>
	requires(std::is_convertible_v<T*, U*>)
inline constexpr bool Ref<T>::operator==(const Ref<U>& other) const
{
	return m_ptr == other.m_ptr;
}

template<typename T>
inline constexpr bool Ref<T>::operator==(std::nullptr_t) const
{
	return m_ptr == nullptr;
}

template<typename T>
template<typename U>
	requires(std::is_convertible_v<T*, U*>)
constexpr std::strong_ordering Ref<T>::operator<=>(const Ref<U>& other) const
{
	return m_ptr <=> other.GetRaw();
}


template<typename T>
constexpr std::strong_ordering Ref<T>::operator<=>(std::nullptr_t) const
{
	return m_ptr <=> reinterpret_cast<ElementType*>(nullptr);
}

template<typename T>
inline constexpr Ref<T>::operator bool() const
{
	return m_ptr != nullptr;
}

template<typename T>
inline void Ref<T>::Reset()
{
	if (m_controlBlock)
	{
		if (m_controlBlock->DecRef())
		{
			m_controlBlock->Destroy(m_ptr);
		}
		m_controlBlock->DecRefControlBlock();

		m_controlBlock = nullptr;
		m_ptr = nullptr;
	}
}

template<typename T>
inline void Ref<T>::Reset(ElementType* initialValue)
{
	ResetInternal(initialValue, DefaultDestroyer<T>());
}

template<typename T>
inline void Ref<T>::Swap(Ref& other)
{
	std::swap(m_ptr, other.m_ptr);
	std::swap(m_controlBlock, other.m_controlBlock);
}

template<typename T>
inline Ref<T>::ElementType* Ref<T>::GetRaw()
{
	return m_ptr;
}

template<typename T>
inline Ref<T>::ElementType* Ref<T>::GetRaw() const
{
	return m_ptr;
}

template<typename T>
template<typename U>
void Ref<T>::CopyInternal(const Ref<U>& other)
{
	Reset();

	m_controlBlock = other.m_controlBlock;

	if (m_controlBlock)
	{
		m_controlBlock->IncRef();
		m_controlBlock->IncRefControlBlock();
	}

	m_ptr = other.m_ptr;
}

template<typename T>
template<typename U>
void Ref<T>::MoveInternal(Ref<U>&& other)
{
	Reset();

	// We inherit the others reference count.
	m_controlBlock = other.m_controlBlock;
	m_ptr = other.m_ptr;

	other.m_controlBlock = nullptr;
	other.m_ptr = nullptr;
}

template<typename T>
template<typename Destroyer>
void Ref<T>::ResetInternal(ElementType* initialValue, Destroyer&& destructor)
{
	Reset();

	m_ptr = initialValue;
	m_controlBlock = new RefControlBlock<ElementType, Destroyer>(std::move(destructor));
}

template<typename T>
constexpr Ref<T>::Ref(T* initialValue, RefControlBlockBase* controlBlock)
	: m_ptr(initialValue),
	m_controlBlock(controlBlock)
{
	if (m_controlBlock)
	{
		m_controlBlock->IncRef();
		m_controlBlock->IncRefControlBlock();
	}
}

namespace std
{
	template<typename T> struct hash;

	template<class Ty>
	struct hash<Ref<Ty>>
	{
		std::size_t operator()(const Ref<Ty> ptr) const
		{
			return std::hash<const void*>()(reinterpret_cast<const void*>(ptr.GetRaw()));
		}
	};
}
