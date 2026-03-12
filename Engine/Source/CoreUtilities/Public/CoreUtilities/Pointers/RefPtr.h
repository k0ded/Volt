#pragma once

#include "CoreUtilities/Malloc.h"
#include <xhash>

template<typename T>
class RefPtr
{
public:
	constexpr RefPtr() noexcept
		: m_object(nullptr)
	{}

	constexpr ~RefPtr() noexcept
	{
		if (m_object)
		{
			m_object->DecRef();
		}
	}

	constexpr RefPtr(std::nullptr_t) noexcept
		: m_object(nullptr)
	{}

	constexpr RefPtr(const RefPtr<T>& other) noexcept
		: m_object(other.m_object)
	{
		if (m_object)
		{
			m_object->IncRef();
		}
	}

	constexpr RefPtr(RefPtr<T>&& other) noexcept
		: m_object(other.m_object)
	{
		other.m_object = nullptr;
	}

	template<class U>
		requires (std::is_convertible_v<U*, T*>)
	constexpr RefPtr(const RefPtr<U>& other) noexcept
		: m_object(static_cast<T*>(other.m_object))
	{
		if (m_object)
		{
			m_object->IncRef();
		}
	}

	template<class U>
		requires (std::is_convertible_v<U*, T*>)
	constexpr RefPtr(RefPtr<U>&& other) noexcept
		: m_object(static_cast<T*>(other.m_object))
	{
		other.m_object = nullptr;
	}

	constexpr RefPtr<T>& operator=(const RefPtr<T>& other) noexcept
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

	template<class U>
		requires (std::is_convertible_v<U*, T*>)
	constexpr RefPtr<T>& operator=(const RefPtr<U>& other) noexcept
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

	constexpr RefPtr<T>& operator=(RefPtr<T>&& other) noexcept
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

	template<class U>
		requires (std::is_convertible_v<U*, T*>)
	constexpr RefPtr<T>& operator=(RefPtr<U>&& other) noexcept
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

	constexpr RefPtr<T>& operator=(nullptr_t) noexcept
	{
		if (m_object)
		{
			m_object->DecRef();
		}

		m_object = nullptr;
		return *this;
	}

	constexpr T* Release() noexcept
	{
		T* ptr = m_object;
		m_object = nullptr;
		return ptr;
	}

	constexpr void Reset() noexcept
	{
		if (m_object)
		{
			m_object->DecRef();
		}

		m_object = nullptr;
	}

	constexpr T* GetRaw() noexcept
	{
		return m_object;
	}

	constexpr T* GetRaw() const noexcept
	{
		return m_object;
	}

	constexpr size_t GetHash() const
	{
		return std::hash<void*>()(m_object);
	}

	template<typename U>
	constexpr RefPtr<U> As() const noexcept
	{
		return RefPtr<U>::Attach(static_cast<U*>(m_object));
	}

	constexpr T* operator->() noexcept
	{
		return m_object;
	}

	constexpr T& operator*() noexcept
	{
		return *m_object;
	}

	constexpr T* operator->() const noexcept
	{
		return m_object;
	}

	constexpr T& operator*() const noexcept
	{
		return *m_object;
	}

	friend constexpr bool operator==(const RefPtr<T>& lhs, const RefPtr<T>& rhs) noexcept
	{
		return lhs.m_object == rhs.m_object;
	}

	template<typename U>
	friend constexpr bool operator==(const RefPtr<T>& lhs, const RefPtr<U>& rhs) noexcept
	{
		return lhs.m_object == rhs.m_object;
	}

	template<typename U>
	friend constexpr bool operator==(const RefPtr<T>& lhs, const U* rhs) noexcept
	{
		return lhs.m_object == rhs;
	}

	template<typename U>
	friend constexpr bool operator==(const U* lhs, const RefPtr<T>& rhs) noexcept
	{
		return lhs == rhs.m_object;
	}

	friend constexpr bool operator==(const RefPtr<T>& lhs, std::nullptr_t) noexcept
	{
		return lhs.m_object == nullptr;
	}

	friend constexpr bool operator==(std::nullptr_t, const RefPtr<T>& rhs) noexcept
	{
		return rhs.m_object == nullptr;
	}

	template<typename U>
	friend constexpr bool operator!=(const RefPtr<T>& lhs, const RefPtr<U>& rhs) noexcept
	{
		return lhs.m_object != rhs.m_object;
	}

	template<typename U>
	friend constexpr bool operator!=(const RefPtr<T>& lhs, const U* rhs) noexcept
	{
		return lhs.m_object != rhs;
	}

	template<typename U>
	friend constexpr bool operator!=(const U* lhs, const RefPtr<T>& rhs) noexcept
	{
		return lhs != rhs.m_object;
	}

	friend constexpr bool operator!=(const RefPtr<T>& lhs, std::nullptr_t) noexcept
	{
		return lhs.m_object != nullptr;
	}

	friend constexpr bool operator!=(std::nullptr_t, const RefPtr<T>& rhs) noexcept
	{
		return rhs.m_object != nullptr;
	}

	VT_NODISCARD VT_INLINE operator bool() const { return m_object != nullptr; }

	template<typename... Args>
	VT_INLINE static RefPtr<T> Create(Args&&... args)
	{
		void* allocatedPtr = Memory::Malloc(sizeof(T), alignof(T));
		T* objectPtr = new (allocatedPtr) T(std::forward<Args>(args)...);
		return RefPtr<T>(objectPtr);
	}

	VT_INLINE static RefPtr<T> Attach(T* ptr)
	{
		RefPtr<T> refPtr;
		refPtr.m_object = ptr;
		if (ptr)
		{
			ptr->IncRef();
		}
		return refPtr;
	}

	VT_INLINE static RefPtr<T> AttachNoRef(T* ptr)
	{
		RefPtr<T> refPtr;
		refPtr.m_object = ptr;
		return refPtr;
	}

private:
	explicit constexpr RefPtr(T* object) noexcept
		: m_object(object)
	{
	}

	template<typename U>
	friend class RefPtr;

	T* m_object;
};

namespace std
{
	template<typename T> struct hash;

	template<class Ty>
	struct hash<RefPtr<Ty>>
	{
		std::size_t operator()(const RefPtr<Ty>& ptr) const
		{
			return std::hash<void*>()(ptr.GetRaw());
		}
	};
}
