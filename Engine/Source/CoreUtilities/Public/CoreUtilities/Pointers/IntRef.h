#pragma once

#include "CoreUtilities/Malloc.h"
#include <xhash>

/*
	A holder class for internally reference counted objects.
	A class can inherit from IntRefCounted for the required implementation.
*/
template<typename T>
class IntRef
{
public:
	constexpr IntRef() noexcept;
	constexpr ~IntRef() noexcept;

	constexpr IntRef(std::nullptr_t) noexcept;
	constexpr IntRef<T>& operator=(nullptr_t) noexcept;

	constexpr IntRef(const IntRef<T>& other) noexcept;
	constexpr IntRef<T>& operator=(const IntRef<T>& other) noexcept;

	constexpr IntRef(IntRef<T>&& other) noexcept;
	constexpr IntRef<T>& operator=(IntRef<T>&& other) noexcept;

	template<class U>
		requires (std::is_convertible_v<U*, T*>)
	constexpr IntRef(const IntRef<U>& other) noexcept;

	template<class U>
		requires (std::is_convertible_v<U*, T*>)
	constexpr IntRef<T>& operator=(const IntRef<U>& other) noexcept;

	template<class U>
		requires (std::is_convertible_v<U*, T*>)
	constexpr IntRef(IntRef<U>&& other) noexcept;

	template<class U>
		requires (std::is_convertible_v<U*, T*>)
	constexpr IntRef<T>& operator=(IntRef<U>&& other) noexcept;

	constexpr T* Release() noexcept;
	constexpr void Reset() noexcept;

	constexpr T* GetRaw() noexcept;
	constexpr T* GetRaw() const noexcept;

	constexpr size_t GetHash() const;

	template<typename U>
	constexpr IntRef<U> As() const noexcept;

	constexpr T* operator->() noexcept;
	constexpr T& operator*() noexcept;

	constexpr T* operator->() const noexcept;
	constexpr T& operator*() const noexcept;

	constexpr bool operator==(const IntRef<T>& rhs) const noexcept;
	constexpr bool operator==(std::nullptr_t) const noexcept;

	template<typename U>
	constexpr bool operator==(const IntRef<U>& rhs) const noexcept;

	template<typename U>
	constexpr bool operator==(const U* rhs) const noexcept;

	VT_NODISCARD VT_INLINE operator bool() const { return m_object != nullptr; }

	template<typename... Args>
	static IntRef<T> Create(Args&&... args);

	static IntRef<T> Attach(T* ptr);
	static IntRef<T> AttachNoRef(T* ptr);

private:
	explicit constexpr IntRef(T* object) noexcept
		: m_object(object)
	{
	}

	template<typename U>
	friend class IntRef;

	T* m_object;
};

namespace std
{
	template<typename T> struct hash;

	template<class Ty>
	struct hash<IntRef<Ty>>
	{
		std::size_t operator()(const IntRef<Ty>& ptr) const
		{
			return std::hash<void*>()(ptr.GetRaw());
		}
	};
}

#include "CoreUtilities/Pointers/IntRef.inl"
