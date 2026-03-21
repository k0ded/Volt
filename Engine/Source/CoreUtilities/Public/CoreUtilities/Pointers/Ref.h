#pragma once

#include "CoreUtilities/VoltAssert.h"
#include "CoreUtilities/Memory.h"

#include <atomic>
#include <compare>

struct RefControlBlockBase
{
	virtual ~RefControlBlockBase() = default;
	virtual void Destroy(void* ptr) = 0;

	std::atomic_int32_t controlBlockRefCount = 1;
	std::atomic_int32_t referenceCount = 1;

	VT_INLINE void IncRefControlBlock() noexcept
	{
		VT_MAYBE_UNUSED int32_t oldValue = controlBlockRefCount.fetch_add(1, std::memory_order::relaxed);
		VT_ASSERT(oldValue > 0);
	}

	VT_INLINE void DecRefControlBlock() noexcept
	{
		int32_t oldValue = controlBlockRefCount.fetch_sub(1, std::memory_order::release);
		VT_ASSERT(oldValue > 0);

		if (oldValue == 1)
		{
			std::atomic_thread_fence(std::memory_order::acquire);
			delete this;
		}
	}

	VT_INLINE void IncRef() noexcept
	{
		VT_MAYBE_UNUSED int32_t oldValue = referenceCount.fetch_add(1, std::memory_order::relaxed);
		VT_ASSERT(oldValue > 0);
	}

	VT_INLINE bool DecRef() noexcept
	{
		int32_t oldValue = referenceCount.fetch_sub(1, std::memory_order::release);
		VT_ASSERT(oldValue > 0);

		if (oldValue == 1)
		{
			std::atomic_thread_fence(std::memory_order::acquire);
			return true;
		}

		return false;
	}

	VT_INLINE int32_t GetRefCount() const noexcept
	{
		return referenceCount.load(std::memory_order::acquire);
	}
};

template<typename T, typename Destroyer>
struct RefControlBlock : public RefControlBlockBase
{
	RefControlBlock(Destroyer&& inDestroyer)
		: destroyer(inDestroyer)
	{}
	~RefControlBlock() override = default;

	void Destroy(void* ptr) override
	{
		T* typedPtr = reinterpret_cast<T*>(ptr);
		destroyer(typedPtr);
	}

	Destroyer destroyer;
};

template<typename T>
class Ref
{
public:
	using ElementType = std::conditional_t<std::is_array_v<T>, std::remove_extent_t<T>, T>;

	constexpr Ref();
	constexpr ~Ref();

	constexpr Ref(const Ref& other);
	constexpr Ref& operator=(const Ref& other);

	constexpr Ref(Ref&& other) noexcept;
	constexpr Ref& operator=(Ref&& other) noexcept;

	template<typename U>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Ref(const Ref<U>& other);

	template<typename U>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Ref& operator=(const Ref<U>& other);

	template<typename U>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Ref(Ref<U>&& other) noexcept;

	template<typename U>
		requires(std::is_convertible_v<U*, T*>)
	constexpr Ref& operator=(Ref<U>&& other) noexcept;

	// No requires clause for these, since they are used for reinterpret cast.
	template<typename U>
	constexpr Ref(Ref<U>&& other, T* initialPtr) noexcept;

	template<typename U>
	constexpr Ref(const Ref<U>& other, T* initialPtr);

	constexpr Ref(ElementType* initialValue);
	
	template<typename Destroyer>
	constexpr Ref(ElementType* initialValue, Destroyer&& destructor);

	constexpr Ref(std::nullptr_t) noexcept;
	constexpr Ref& operator=(std::nullptr_t) noexcept;

	constexpr T* operator->() noexcept;
	constexpr T& operator*() noexcept;
	constexpr T* operator->() const noexcept;
	constexpr T& operator*() const noexcept;

	template<typename U>
		requires(std::is_convertible_v<T*, U*>)
	constexpr bool operator==(const Ref<U>& other) const;

	template<typename U>
		requires(std::is_convertible_v<T*, U*>)
	constexpr std::strong_ordering operator<=>(const Ref<U>& other) const;

	constexpr bool operator==(std::nullptr_t) const;
	constexpr std::strong_ordering operator<=>(std::nullptr_t) const;

	constexpr operator bool() const;

	void Reset();
	void Reset(ElementType* initialValue);
	void Swap(Ref& other);

	ElementType* GetRaw();
	ElementType* GetRaw() const;

private:
	template<typename U>
	friend class Ref;

	template<typename U>
	friend class Weak;

	constexpr Ref(T* initialValue, RefControlBlockBase* controlBlock);

	template<typename U>
	void CopyInternal(const Ref<U>& other);

	template<typename U>
	void MoveInternal(Ref<U>&& other);

	template<typename Destroyer>
	void ResetInternal(ElementType* initialValue, Destroyer&& destructor);

	ElementType* m_ptr = nullptr;
	mutable RefControlBlockBase* m_controlBlock = nullptr;
};

template<typename T, typename... Args>
VT_NODISCARD Ref<T> CreateRef(Args&&... args)
{
	return Ref<T>(new T(std::forward<Args>(args)...));
}

template<typename T, typename U>
VT_NODISCARD Ref<T> ReinterpretRefCast(const Ref<U>& other)
{
	using ToType = typename Ref<T>::ElementType;
	return Ref<T>(other, reinterpret_cast<ToType*>(other.GetRaw()));
}

template<typename T, typename U>
VT_NODISCARD Ref<T> ReinterpretRefCast(Ref<U>&& other)
{
	using ToType = typename Ref<T>::ElementType;
	return Ref<T>(std::move(other), reinterpret_cast<ToType*>(other.GetRaw()));
}

template<typename T, typename U>
VT_NODISCARD Ref<T> StaticRefCast(const Ref<U>& other)
{
	using ToType = typename Ref<T>::ElementType;
	return Ref<T>(other, static_cast<ToType*>(other.GetRaw()));
}

template<typename T, typename U>
VT_NODISCARD Ref<T> StaticRefCast(Ref<U>&& other)
{
	using ToType = typename Ref<T>::ElementType;
	return Ref<T>(std::move(other), static_cast<ToType*>(other.GetRaw()));
}

#include "CoreUtilities/Pointers/Ref.inl"
