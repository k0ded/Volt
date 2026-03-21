#pragma once

#include "CoreUtilities/Pointers/Ref.h"

template<typename T>
class Weak
{
public:
	using ElementType = std::conditional_t<std::is_array_v<T>, std::remove_extent_t<T>, T>;

	constexpr Weak();
	constexpr ~Weak();

	constexpr Weak(const Weak& other);
	constexpr Weak& operator=(const Weak& other);

	constexpr Weak(Weak&& other) noexcept;
	constexpr Weak& operator=(Weak&& other) noexcept;

	template<typename U>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Weak(const Weak<U>& other);

	template<typename U>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Weak& operator=(const Weak<U>& other);

	template<typename U>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Weak(Weak<U>&& other) noexcept;

	template<typename U>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Weak& operator=(Weak<U>&& other) noexcept;

	constexpr Weak(const Ref<T>& other);
	constexpr Weak& operator=(const Ref<T>& other);

	template<typename U>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Weak(const Ref<U>& other);

	template<typename U>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Weak& operator=(const Ref<U>& other);

	template<typename U>
		requires(std::is_convertible_v<T*, U*>)
	constexpr std::strong_ordering operator<=>(const Weak<U>& other) const;

	constexpr bool operator==(std::nullptr_t) const;
	constexpr std::strong_ordering operator<=>(std::nullptr_t) const;

	constexpr void Reset();
	constexpr void Swap(Weak& other);
	constexpr Ref<T> Lock() const;

	constexpr bool IsExpired() const;

private:
	template<typename U>
	friend class Weak;

	template<typename U>
	void CopyInternal(const Weak<U>& other);

	template<typename U>
	void MoveInternal(Weak<U>&& other);

	T* m_ptr = nullptr;
	mutable RefControlBlockBase* m_controlBlock = nullptr;
};

#include "CoreUtilities/Pointers/Weak.inl"
