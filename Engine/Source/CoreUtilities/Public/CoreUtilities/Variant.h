#pragma once

#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/VoltAssert.h"

#include <cstdint>
#include <type_traits>
#include <xutility>

template<typename T, typename... Ts>
concept IsInVariant = (std::is_same_v<std::decay_t<T>, Ts> || ...);

template<typename... Ts>
class Variant
{
public:
	static constexpr size_t NPos = static_cast<size_t>(-1);

	VT_INLINE Variant() noexcept;
	VT_INLINE ~Variant() noexcept;

	template<typename T>
	requires (!std::is_same_v<std::decay_t<T>, Variant<Ts...>> &&
		IsInVariant<T, Ts...>)
	VT_INLINE Variant(T&& value) noexcept;

	VT_INLINE Variant(const Variant& other) noexcept;
	VT_INLINE Variant(Variant&& other) noexcept;

	VT_INLINE Variant& operator=(const Variant& other) noexcept;
	VT_INLINE Variant& operator=(Variant&& other) noexcept;

	template<typename T>
	requires (!std::is_same_v<std::decay_t<T>, Variant<Ts...>>&&
		IsInVariant<T, Ts...>)
	VT_INLINE Variant& operator=(T&& value) noexcept;

	template<typename T, typename... Args>
	VT_INLINE T& Emplace(Args&&... args);

	template<typename T>
	VT_INLINE T& Get();

	template<typename T>
	VT_INLINE const T& Get() const;

	template<typename T>
	VT_INLINE bool Is() const;

	VT_NODISCARD VT_INLINE bool IsValid() const;

private:
	static constexpr size_t MaxSize = std::max({ sizeof(Ts)... });
	static constexpr size_t MaxAlignment = std::max({ alignof(Ts)... });

	VT_INLINE void Destroy();

	VT_INLINE void CopyInto(std::byte* dstStorage, size_t& dstIndex) const;
	template<size_t I> void CopyRecursive(std::byte* dstStorage, size_t& dstIndex) const;

	VT_INLINE void MoveInto(std::byte* dstStorage, size_t& dstIndex) const;
	template<size_t I> void MoveRecursive(std::byte* dstStorage, size_t& dstIndex) const;

	VT_INLINE static void DestroyAtIndex(size_t index, std::byte* storage);
	template<size_t I> static void DestroyRecursive(size_t index, std::byte* storage);

	template<typename T, size_t I = 0>
	static constexpr size_t IndexOf();

	alignas(MaxAlignment) std::byte m_storage[MaxSize];
	size_t m_typeIndex;
};

template<typename... Ts>
VT_INLINE Variant<Ts...>::Variant() noexcept
	: m_typeIndex(NPos)
{
}

template<typename... Ts>
VT_INLINE Variant<Ts...>::~Variant() noexcept
{
	Destroy();
}

template<typename... Ts>
template<typename T>
	requires (!std::is_same_v<std::decay_t<T>, Variant<Ts...>> &&
		IsInVariant<T, Ts...>)
VT_INLINE Variant<Ts...>::Variant(T&& value) noexcept
{
	Emplace<std::decay_t<T>>(std::forward<T>(value));
}

template<typename... Ts>
VT_INLINE Variant<Ts...>::Variant(const Variant& other) noexcept
{
	if (other.IsValid())
	{
		other.CopyInto(m_storage, m_typeIndex);
	}
	else
	{
		m_typeIndex = NPos;
	}
}

template<typename... Ts>
VT_INLINE Variant<Ts...>::Variant(Variant&& other) noexcept
{
	if (other.IsValid())
	{
		other.MoveInto(m_storage, m_typeIndex);
	}
	else
	{
		m_typeIndex = NPos;
	}
}

template<typename... Ts>
VT_INLINE Variant<Ts...>& Variant<Ts...>::operator=(const Variant& other) noexcept
{
	if (this != &other)
	{
		if (other.IsValid())
		{
			other.CopyInto(m_storage, m_typeIndex);
		}
		else
		{
			m_typeIndex = NPos;
		}
	}

	return *this;
}

template<typename... Ts>
VT_INLINE Variant<Ts...>& Variant<Ts...>::operator=(Variant&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		if (other.IsValid())
		{
			other.MoveInto(m_storage, m_typeIndex);
		}
		else
		{
			m_typeIndex = NPos;
		}
	}

	return *this;
}

template<typename... Ts>
template<typename T>
requires (!std::is_same_v<std::decay_t<T>, Variant<Ts...>>&&
	IsInVariant<T, Ts...>)
VT_INLINE Variant<Ts...>& Variant<Ts...>::operator=(T&& value) noexcept
{
	Emplace<std::decay_t<T>>(std::forward<T>(value));
	return *this;
}

template<typename... Ts>
template<typename T, typename... Args>
VT_INLINE T& Variant<Ts...>::Emplace(Args&&... args)
{
	Destroy();

	new(&m_storage) T(std::forward<Args>(args)...);
	m_typeIndex = IndexOf<T>();

	return *reinterpret_cast<T*>(&m_storage);
}

template<typename... Ts>
VT_INLINE void Variant<Ts...>::Destroy()
{
	if (!IsValid())
	{
		return;
	}

	DestroyAtIndex(m_typeIndex, m_storage);
	m_typeIndex = NPos;
}

template<typename... Ts>
VT_NODISCARD VT_INLINE bool Variant<Ts...>::IsValid() const
{
	return m_typeIndex != NPos;
}

template<typename...Ts>
VT_INLINE void Variant<Ts...>::DestroyAtIndex(size_t index, std::byte* storage)
{
	DestroyRecursive<0>(index, storage);
}

template<typename... Ts>
template<size_t I>
VT_INLINE void Variant<Ts...>::DestroyRecursive(size_t index, std::byte* storage)
{
	if constexpr (I < sizeof...(Ts))
	{
		if (index == I)
		{
			using T = std::tuple_element_t<I, std::tuple<Ts...>>;

			reinterpret_cast<T*>(storage)->~T();
		}
		else
		{
			DestroyRecursive<I + 1>(index, storage);
		}
	}
}

template<typename... Ts>
template<typename T, size_t I /*= 0*/>
constexpr size_t Variant<Ts...>::IndexOf()
{
	using U = std::tuple_element_t<I, std::tuple<Ts...>>;
	if constexpr (std::is_same_v<T, U>)
	{
		return I;
	}
	else if constexpr (I + 1 < sizeof...(Ts))
	{
		return IndexOf<T, I + 1>();
	}
	else
	{
		static_assert(!sizeof(T), "Type not in Variant!");
	}
}

template<typename... Ts>
template<typename T>
VT_INLINE T& Variant<Ts...>::Get()
{
	VT_ENSURE(m_typeIndex == IndexOf<T>());
	return *reinterpret_cast<T*>(&m_storage);
}

template<typename... Ts>
template<typename T>
VT_INLINE const T& Variant<Ts...>::Get() const
{
	VT_ENSURE(m_typeIndex == IndexOf<T>());
	return *reinterpret_cast<const T*>(&m_storage);
}

template<typename... Ts>
template<typename T>
VT_INLINE bool Variant<Ts...>::Is() const
{
	return m_typeIndex == IndexOf<T>();
}

template<typename...Ts>
VT_INLINE void Variant<Ts...>::CopyInto(std::byte* dstStorage, size_t& dstIndex) const
{
	CopyRecursive<0>(dstStorage, dstIndex);
}

template<typename...Ts>
template<size_t I>
void Variant<Ts...>::CopyRecursive(std::byte* dstStorage, size_t& dstIndex) const
{
	if constexpr (I < sizeof...(Ts))
	{
		if (m_typeIndex == I)
		{
			using T = std::tuple_element_t<I, std::tuple<Ts...>>;
			new (dstStorage) T(Get<T>());

			dstIndex = I;
		}
		else
		{
			CopyRecursive<I + 1>(dstStorage, dstIndex);
		}
	}
}

template<typename...Ts>
VT_INLINE void Variant<Ts...>::MoveInto(std::byte* dstStorage, size_t& dstIndex) const
{
	MoveRecursive<0>(dstStorage, dstIndex);
}

template<typename...Ts>
template<size_t I>
void Variant<Ts...>::MoveRecursive(std::byte* dstStorage, size_t& dstIndex) const
{
	if constexpr (I < sizeof...(Ts))
	{
		if (m_typeIndex == I)
		{
			using T = std::tuple_element_t<I, std::tuple<Ts...>>;
			new (dstStorage) T(std::move(Get<T>()));

			dstIndex = I;
		}
		else
		{
			MoveRecursive<I + 1>(dstStorage, dstIndex);
		}
	}
}
