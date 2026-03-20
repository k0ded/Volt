#pragma once

#include <CoreUtilities/CompilerTraits.h>

#include <type_traits>

template<typename T>
struct DefaultDestructor
{
	void operator()(T* object) const
	{
		delete object;
	}
};

template<typename T, typename DestructorType = DefaultDestructor<T>>
class Unique
{
public:
	constexpr Unique();
	constexpr ~Unique();

	constexpr Unique(const Unique& other) = delete;
	constexpr Unique& operator=(const Unique& other) = delete;

	constexpr Unique(Unique&& other) noexcept;
	constexpr Unique& operator=(Unique&& other) noexcept;

	template<typename U, typename UDestructorType>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Unique(Unique<U, UDestructorType>&& other) noexcept;

	template<typename U, typename UDestructorType>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Unique& operator=(Unique<U, UDestructorType>&& other) noexcept;

	constexpr Unique(T* initalValue);

	constexpr Unique(std::nullptr_t) noexcept;
	constexpr Unique& operator=(std::nullptr_t) noexcept;

	constexpr T* operator->() noexcept;
	constexpr T& operator*() noexcept;
	constexpr T* operator->() const noexcept;
	constexpr T& operator*() const noexcept;

	constexpr bool operator==(const Unique& other) const;
	constexpr bool operator==(std::nullptr_t) const;
	constexpr operator bool() const;

	void Reset();
	void Reset(T* initialValue);

	T* GetRaw();
	T* GetRaw() const;

private:
	template<typename U, typename DestructorType>
	friend class Unique;

	T* m_ptr;
	DestructorType m_destructor;
};

template<typename T, typename... Args>
VT_NODISCARD Unique<T> CreateUnique(Args&&... args)
{
	return Unique<T>(new T(std::forward<Args>(args)...));
}

#include "CoreUtilities/Pointers/Unique.inl"
