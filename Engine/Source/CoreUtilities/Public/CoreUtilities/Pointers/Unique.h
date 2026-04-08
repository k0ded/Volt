#pragma once

#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/Memory.h"

#include <type_traits>

template<typename T, typename DestroyerType = DefaultDestroyer<T>>
class Unique
{
public:
	constexpr Unique();
	constexpr ~Unique();

	constexpr Unique(const Unique& other) = delete;
	constexpr Unique& operator=(const Unique& other) = delete;

	constexpr Unique(Unique&& other) noexcept;
	constexpr Unique& operator=(Unique&& other) noexcept;

	template<typename U, typename UDestroyerType>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Unique(Unique<U, UDestroyerType>&& other) noexcept;

	template<typename U, typename UDestroyerType>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Unique& operator=(Unique<U, UDestroyerType>&& other) noexcept;

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
	template<typename U, typename OtherDestroyerType>
	friend class Unique;

	T* m_ptr;
	DestroyerType m_destructor;
};

template<typename T, typename... Args>
VT_NODISCARD Unique<T> CreateUnique(Args&&... args)
{
	return Unique<T>(new T(std::forward<Args>(args)...));
}

#include "CoreUtilities/Pointers/Unique.inl"
