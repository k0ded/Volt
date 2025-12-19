#pragma once

#include "CoreUtilities/Core.h"
#include "CoreUtilities/TypeTraits/TypeIndex.h"

class Any
{
public:
	Any() noexcept = default;

	template<typename T>
	Any(T value)
	{
		Emplace(value);
	}

	Any(const Any& other)
	{
		CopyFrom(other);
	}

	Any(Any&& other)
	{
		MoveFrom(std::move(other));
	}

	Any& operator=(const Any& other)
	{
		if (this != &other)
		{
			CopyFrom(other);
		}

		return *this;
	}

	Any& operator=(Any&& other)
	{
		if (this != &other)
		{
			MoveFrom(std::move(other));
		}

		return *this;
	}

	~Any()
	{
		Reset();
	}

	bool HasValue() const
	{
		return m_ops != nullptr;
	}

	void Reset()
	{
		if (m_ops && m_dataPtr != nullptr)
		{
			m_ops->Destroy(m_dataPtr);
			if (!m_isInlineStorage)
			{
				free(m_storage.heapData);
			}
		}

		m_ops = nullptr;
		m_dataPtr = nullptr;
		m_isInlineStorage = false;
	}

	const TypeTraits::TypeIndex GetType() const
	{
		constexpr TypeTraits::TypeIndex VoidType = TypeTraits::TypeIndex::FromType<void>();
		if (m_ops)
		{
			return m_ops->Type();
		}

		return VoidType;
	}

	template<typename T>
	void Emplace(T value)
	{
		Reset();

		using U = std::decay_t<T>;
		m_ops = &AnyOpsImpl<U>::Ops;

		if constexpr (FitsInlineStorage<U>())
		{
			new (m_storage.buffer) U(std::move(value));
			m_dataPtr = m_storage.buffer;
			m_isInlineStorage = true;
		}
		else
		{
			m_storage.heapData = new U(std::move(value));
			m_dataPtr = m_storage.heapData;
			m_isInlineStorage = false;
		}
	}

	template<typename T>
	T& Cast()
	{
		constexpr TypeTraits::TypeIndex CastType = TypeTraits::TypeIndex::FromType<T>();

		VT_ENSURE(HasValue());
		VT_ENSURE(GetType() == CastType);

		return *static_cast<T*>(m_dataPtr);
	}

	template<typename T>
	const T& Cast() const
	{
		constexpr TypeTraits::TypeIndex CastType = TypeTraits::TypeIndex::FromType<T>();

		VT_ENSURE(HasValue());
		VT_ENSURE(GetType() == CastType);

		return *static_cast<T*>(m_dataPtr);
	}

private:
	template<typename T>
	static consteval bool FitsInlineStorage()
	{
		return sizeof(T) <= AnyStorage::AnyBufferSize &&
			alignof(T) <= AnyStorage::AnyBufferAlign &&
			std::is_nothrow_move_constructible_v<T>;
	}

	struct AnyStorage
	{
		inline static constexpr size_t AnyBufferSize = 32;
		inline static constexpr size_t AnyBufferAlign = alignof(std::max_align_t);

		alignas(AnyBufferAlign)
		uint8_t buffer[AnyBufferSize];

		void* heapData = nullptr;
	};

	struct AnyOps
	{
		void* (*Allocate)();
		void (*Destroy)(void*);
		void (*Copy)(void* dst, const void* src);
		void (*Move)(void* dst, void* src);
		TypeTraits::TypeIndex(*Type)();
	};

	template<typename T>
	struct AnyOpsImpl
	{
		static void* Allocate()
		{
			return malloc(sizeof(T));
		}

		static void Destroy(void* ptr)
		{
			reinterpret_cast<T*>(ptr)->~T();
		}

		static void Copy(void* dst, const void* src)
		{
			new (dst) T(*reinterpret_cast<const T*>(src));
		}

		static void Move(void* dst, void* src)
		{
			new (dst) T(std::move(*reinterpret_cast<T*>(src)));
			Destroy(src);
		}

		static TypeTraits::TypeIndex Type()
		{
			constexpr TypeTraits::TypeIndex ValueTypeIndex = TypeTraits::TypeIndex::FromType<T>();
			return ValueTypeIndex;
		}

		inline static AnyOps Ops = 
		{
			&Allocate,
			&Destroy,
			&Copy,
			&Move,
			&Type
		};
	};

	void CopyFrom(const Any& other)
	{
		if (!other.m_ops)
		{
			return;
		}

		m_ops = other.m_ops;
		m_isInlineStorage = other.m_isInlineStorage;

		if (m_isInlineStorage)
		{
			m_ops->Copy(m_storage.buffer, other.m_storage.buffer);
			m_dataPtr = m_storage.buffer;
		}
		else
		{
			m_storage.heapData = m_ops->Allocate();
			m_ops->Copy(m_storage.heapData, other.m_storage.heapData);
			m_dataPtr = m_storage.heapData;
		}
	}

	void MoveFrom(Any&& other)
	{
		if (!other.m_ops)
		{
			return;
		}

		m_ops = other.m_ops;
		m_isInlineStorage = other.m_isInlineStorage;

		if (m_isInlineStorage)
		{
			m_ops->Move(m_storage.buffer, other.m_storage.buffer);
			m_dataPtr = m_storage.buffer;
		}
		else
		{
			m_storage.heapData = other.m_storage.heapData;
			m_dataPtr = m_storage.heapData;
			other.m_storage.heapData = nullptr;
		}

		other.m_ops = nullptr;
		other.m_dataPtr = nullptr;
		other.m_isInlineStorage = false;
	}

	AnyStorage m_storage;
	const AnyOps* m_ops = nullptr;
	void* m_dataPtr = nullptr;
	bool m_isInlineStorage = false;
};
