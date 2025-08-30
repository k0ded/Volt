#pragma once

#include <xhash>

template<typename T>
class RefPtr
{
public:
	constexpr RefPtr() noexcept
		: m_object(nullptr)
	{ }

	constexpr ~RefPtr() noexcept
	{
		Reset();
	}

	constexpr RefPtr(std::nullptr_t) noexcept
		: m_object(nullptr)
	{ }

	constexpr RefPtr(const RefPtr<T>& other) noexcept
		: m_object(other.m_object)
	{
		if (m_object)
		{
			m_object->IncRef();
		}
	}

	constexpr RefPtr(RefPtr<T>&& other) noexcept
		: m_object(other.Release())
	{}

	constexpr RefPtr<T>& operator=(const RefPtr<T>& other) noexcept
	{
		T* temp = m_object;
		m_object = other.m_object;

		if (m_object)
		{
			m_object->IncRef();
		}

		if (temp)
		{
			temp->DecRef();
		}

		return *this;
	}

	constexpr RefPtr<T>& operator=(RefPtr<T>&& other) noexcept
	{
		T* newVal = other.Release();
		T* oldVal = m_object;

		m_object = newVal;

		if (oldVal)
		{
			oldVal->DecRef();
		}

		return *this;
	}

	template<class U, class = std::enable_if<std::is_convertible_v<U*, T*>, void>>
	constexpr RefPtr(const RefPtr<U>& other) noexcept
		: m_object(reinterpret_cast<T*>(other.GetRaw()))
	{
		if (m_object)
		{
			m_object->IncRef();
		}
	}

	template<class U, class = std::enable_if<std::is_convertible_v<U*, T*>, void>>
	constexpr RefPtr(RefPtr<U>&& other) noexcept
		: m_object(reinterpret_cast<T*>(other.Release()))
	{}

	template<class U, class = std::enable_if<std::is_convertible_v<U*, T*>, void>>
	constexpr RefPtr<T>& operator=(const RefPtr<U>& other) noexcept
	{
		T* temp = m_object;
		m_object = other.GetRaw();

		if (m_object)
		{
			m_object->IncRef();
		}

		if (temp)
		{
			temp->DecRef();
		}

		return *this;
	}

	template<class U, class = std::enable_if<std::is_convertible_v<U*, T*>, void>>
	constexpr RefPtr<T>& operator=(RefPtr<U>&& other) noexcept
	{
		if (m_object)
		{
			m_object->DecRef();
		}
		m_object = other.Release();

		return *this;
	}

	constexpr RefPtr<T>& operator=(nullptr_t null) noexcept
	{
		Reset();
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
	constexpr RefPtr<U> As() const noexcept requires (std::is_base_of<T, U>::value || std::is_base_of<U, T>::value)
	{
		return RefPtr<U>(const_cast<RefPtr<T>&>(*this));
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
		return lhs.m_object == rhs.GetRaw();
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
		return nullptr == rhs.m_object;
	}

	template<typename U>
	friend constexpr bool operator!=(const RefPtr<T>& lhs, const RefPtr<U>& rhs) noexcept
	{
		return !(lhs.m_object == rhs.GetRaw());
	}

	template<typename U>
	friend constexpr bool operator!=(const RefPtr<T>& lhs, const U* rhs) noexcept
	{
		return  !(lhs.m_object == rhs);
	}

	template<typename U>
	friend constexpr bool operator!=(const U* lhs, const RefPtr<T>& rhs) noexcept
	{
		return !(lhs == rhs.m_object);
	}

	friend constexpr bool operator!=(const RefPtr<T>& lhs, std::nullptr_t) noexcept
	{
		return !(lhs.m_object == nullptr);
	}

	friend constexpr bool operator!=(std::nullptr_t, const RefPtr<T>& rhs) noexcept
	{
		return !(nullptr == rhs.m_object);
	}

	VT_NODISCARD VT_INLINE operator bool() const { return m_object != nullptr; }

	template<typename... Args>
	VT_INLINE static RefPtr<T> Create(Args&&... args)
	{
		typename T::Allocator allocator;

		void* allocatedPtr = allocator.Allocate(sizeof(T), alignof(T));
		T* objectPtr = new (allocatedPtr) T(std::forward<Args>(args)...);
		return RefPtr<T>(objectPtr);
	}

	VT_INLINE static RefPtr<T> Attach(T* ptr)
	{
		RefPtr<T> refPtr(ptr);
		refPtr.m_object->IncRef();

		return refPtr;
	}

	VT_INLINE static RefPtr<T> AttachNoRef(T* ptr)
	{
		RefPtr<T> refPtr(ptr);
		return refPtr;
	}

private:
	RefPtr(T* object) noexcept
		: m_object(object)
	{}

	T* m_object;
};

namespace std
{
	template<typename T> struct hash;

	template<class Ty>
	struct hash<RefPtr<Ty>>
	{
		std::size_t operator()(const RefPtr<Ty> ptr) const
		{
			return ptr.GetHash();
		}
	};
}
