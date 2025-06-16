#pragma once

#include "RenderCore/Config.h"

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/TypeTraits/TypeIndex.h>

#include <CoreUtilities/Allocators/LinearAllocator.h>

namespace Volt
{
	class VTRC_API RenderGraphBlackboard
	{
	public:
		~RenderGraphBlackboard();

		template<typename T>
		inline T& Add()
		{
			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			VT_ENSURE(!m_typeInfos.contains(typeIndex));

			void* newAllocation = Allocate(sizeof(T));
			new (newAllocation) T();

			auto destructor = [](void* ptr)
			{
				T* typePtr = reinterpret_cast<T*>(ptr);
				typePtr->~T();
			};

			m_typeInfos[typeIndex] = { newAllocation, destructor };
		
			return *reinterpret_cast<T*>(newAllocation);
		}

		template<typename T>
		inline T& Get()
		{
			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();

			VT_ENSURE_MSG(m_typeInfos.contains(typeIndex), "Blackboard does not contain type!");
			return *reinterpret_cast<T*>(m_typeInfos.at(typeIndex).typePtr);
		}

		template<typename T>
		inline const T& Get() const
		{
			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();

			VT_ENSURE_MSG(m_typeInfos.contains(typeIndex), "Blackboard does not contain type!");
			return *reinterpret_cast<const T*>(m_typeInfos.at(typeIndex).typePtr);
		}

		template<typename T>
		inline const bool Contains() const
		{
			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();
			return m_typeInfos.contains(typeIndex);
		}

	private:
		struct TypeInfo
		{
			void* typePtr;
			std::function<void(void* ptr)> typeDestructor;
		};

		void* Allocate(size_t size);

		inline static constexpr size_t BlackboardSize = 2048;

		LinearAllocator<BlackboardSize> m_allocator;
		vt::map<TypeTraits::TypeIndex, TypeInfo> m_typeInfos;
	};
}
