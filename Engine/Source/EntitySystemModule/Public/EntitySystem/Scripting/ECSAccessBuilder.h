#pragma once

#include "EntitySystem/Scripting/CoreComponents.h"
#include "EntitySystem/EntityID.h"
#include "EntitySystem/Entity.h"

#include <entt.hpp>

namespace ECS
{
	namespace Utility
	{
		template<typename T, typename Tuple>
		struct TupleTypeIndexHelper;

		template<typename T>
		struct TupleTypeIndexHelper<T, std::tuple<>>
		{
			static constexpr std::size_t Value = 0;
		};

		template<typename T, typename... Types>
		struct TupleTypeIndexHelper<T, std::tuple<T, Types...>>
		{
			static constexpr std::size_t Value = 0;
		};

		template<typename T, typename U, typename... Types>
		struct TupleTypeIndexHelper<T, std::tuple<U, Types...>>
		{
			static constexpr std::size_t Value = 1 + TupleTypeIndexHelper<T, std::tuple<Types...>>::Value;
		};

		template<typename T, typename Tuple>
		struct TupleTypeIndex
		{
			static constexpr std::size_t Value = TupleTypeIndexHelper<T, Tuple>::Value;
			static constexpr bool IsValid = Value < std::tuple_size_v<Tuple>;
		};
	}

	enum class AccessType
	{
		Read,
		Write,
		With,
		Without,
		ReadIfExists,
		WriteIfExists
	};

	enum class Type
	{
		Entity,
		Query
	};

	template<typename T, AccessType ACCESS_TYPE>
	struct SingleComponentAccess
	{
		typedef T ComponentType;
		inline static constexpr AccessType accessType = ACCESS_TYPE;
	};

	template<typename CompType, bool isWrite>
	struct ComponentTraits
	{
		using Type = std::conditional_t<isWrite, CompType&, const CompType&>;
	};

	template<AccessType... types>
	struct IsNot
	{
		template<typename T>
		struct apply : std::bool_constant<((T::accessType != types) && ...)> {};
	};

	template<AccessType... types>
	struct Is
	{
		template<typename T>
		struct apply : std::bool_constant<((T::accessType == types) && ...)> {};
	};

	using IsWithout = Is<AccessType::Without>;
	using IsNotWithout = IsNot<AccessType::Without>;
	using IsNotWithoutOrWith = IsNot<AccessType::Without, AccessType::With>;
	using IsGuaranteedAccessible = IsNot<AccessType::Without, AccessType::With, AccessType::WriteIfExists, AccessType::ReadIfExists>;

	template<typename Filter, bool RemoveConstRef, typename... Ts>
	struct FilterComponents;

	template<typename Filter, bool RemoveConstRef, typename T, typename... Ts>
	struct FilterComponents<Filter, RemoveConstRef, T, Ts...>
	{
		using Type = std::conditional_t<RemoveConstRef,
			std::conditional_t<
				Filter::template apply<T>::value,
				decltype(std::tuple_cat(
					std::declval<typename FilterComponents<Filter, RemoveConstRef, Ts...>::Type>(),
					std::declval<std::tuple<std::remove_const_t<std::remove_reference_t<typename ComponentTraits<typename T::ComponentType, T::accessType == AccessType::Write || T::accessType == AccessType::WriteIfExists>::Type>>>>()
				)),
				typename FilterComponents<Filter, RemoveConstRef, Ts...>::Type
			>,
				std::conditional_t<
				Filter::template apply<T>::value,
				decltype(std::tuple_cat(
					std::declval<typename FilterComponents<Filter, RemoveConstRef, Ts...>::Type>(),
					std::declval<std::tuple<typename ComponentTraits<typename T::ComponentType, T::accessType == AccessType::Write || T::accessType == AccessType::WriteIfExists>::Type>>()
				)),
				typename FilterComponents<Filter, RemoveConstRef, Ts...>::Type
			>
		>;
	};

	template<typename Filter, bool RemoveConstRef>
	struct FilterComponents<Filter, RemoveConstRef>
	{
		using Type = std::tuple<>;
	};

	template<typename T>
	using RemoveConstRef = std::remove_const_t<std::remove_reference_t<T>>;

	template<typename T, typename ComponentTuple>
	concept ComponentIsSpecifiedInAccessor = Utility::TupleTypeIndex<RemoveConstRef<T>, ComponentTuple>::IsValid;

	template<typename Comp, typename ComponentTuple, typename ComponentTupleWriteIfExists>
	concept IsComponentWriteAccess =
		Utility::TupleTypeIndex<RemoveConstRef<Comp>&, ComponentTuple>::IsValid ||
		Utility::TupleTypeIndex<RemoveConstRef<Comp>, ComponentTupleWriteIfExists>::IsValid;

	template<typename Comp, typename ComponentTuple, typename ComponentTupleReadIfExists>
	concept IsComponentReadAccess =
		Utility::TupleTypeIndex<const RemoveConstRef<Comp>&, ComponentTuple>::IsValid ||
		Utility::TupleTypeIndex<RemoveConstRef<Comp>, ComponentTupleReadIfExists>::IsValid;

	template<Type type, typename... T>
	class ConstructComponents
	{};

	template<typename... T>
	class ConstructComponents<Type::Entity, T...>
	{
	public:
		using ComponentViewTuple = typename FilterComponents<IsNotWithout, false, T...>::Type;
		using ComponentViewExcludeTuple = typename FilterComponents<IsWithout, false, T...>::Type;
		using ComponentTuple = typename FilterComponents<IsNotWithoutOrWith, false, T...>::Type;
		using ComponentTupleRaw = typename FilterComponents<IsNotWithoutOrWith, true, T...>::Type;
		using ComponentTupleStructuredBindings = typename FilterComponents<IsGuaranteedAccessible, false, T...>::Type;

		using ComponentTupleReadIfExists = typename FilterComponents<Is<AccessType::ReadIfExists>, true, T...>::Type;
		using ComponentTupleWriteIfExists = typename FilterComponents<Is<AccessType::WriteIfExists>, true, T...>::Type;

		inline static constexpr Type ConstructType = ::ECS::Type::Entity;

		ConstructComponents(const Volt::Entity& entityHelper)
			: m_entity(entityHelper)
		{
			VT_ENSURE(m_entity);
		}

		template<typename Comp>
		Comp& GetComponent() requires IsComponentWriteAccess<Comp, ComponentTuple, ComponentTupleWriteIfExists>
		{
			using ComponentTraits = Utility::TupleTypeIndex<RemoveConstRef<Comp>, ComponentTupleRaw>;

			if constexpr (ComponentTraits::IsValid)
			{
				return m_entity.GetComponent<Comp>();
			}
			else
			{
				using WriteIfExistsTraits = Utility::TupleTypeIndex<RemoveConstRef<Comp>, ComponentTupleWriteIfExists>;

				static_assert(WriteIfExistsTraits::IsValid);
				return m_entity.GetComponent<Comp>();
			}
		}

		template<typename Comp>
		const Comp& GetComponent() const requires IsComponentReadAccess<Comp, ComponentTuple, ComponentTupleReadIfExists> || IsComponentWriteAccess<Comp, ComponentTuple, ComponentTupleWriteIfExists>
		{
			using ComponentTraits = Utility::TupleTypeIndex<RemoveConstRef<Comp>, ComponentTupleRaw>;

			if constexpr (ComponentTraits::IsValid)
			{
				return m_entity.GetComponent<Comp>();
			}
			else
			{
				using ReadIfExistsTraits = Utility::TupleTypeIndex<RemoveConstRef<Comp>, ComponentTupleReadIfExists>;
				using WriteIfExistsTraits = Utility::TupleTypeIndex<RemoveConstRef<Comp>, ComponentTupleWriteIfExists>;

				static_assert(ReadIfExistsTraits::IsValid && WriteIfExistsTraits::IsValid);
				return m_entity.GetComponent<Comp>();
			}
		}

		template<typename Comp>
		Comp& GetComponentUnsafe()
		{
			return m_entity.GetComponent<Comp>();
		}

		template<typename Comp, typename... Args>
		Comp& AddComponent(Args&&... args)
		{
			return m_entity.AddComponent<Comp>(std::forward<Args>(args)...);
		}

		template<typename Comp>
		bool HasComponent()
		{
			return m_entity.HasComponent<Comp>();
		}

		template<typename Comp>
		void RemoveComponent()
		{
			return m_entity.RemoveComponent<Comp>();
		}

		void SetPosition(const glm::vec3& position) requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			m_entity.SetPosition(position);
		}

		void SetRotation(const glm::quat& rotation) requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			m_entity.SetRotation(rotation);
		}

		void SetScale(const glm::vec3& scale) requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			m_entity.SetScale(scale);
		}

		void SetLocalPosition(const glm::vec3& position) requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			m_entity.SetLocalPosition(position);
		}

		void SetLocalRotation(const glm::quat& rotation) requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			m_entity.SetLocalRotation(rotation);
		}

		void SetLocalScale(const glm::vec3& scale) requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			m_entity.SetLocalScale(scale);
		}

		VT_NODISCARD glm::vec3 GetPosition() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetPosition();
		}

		VT_NODISCARD glm::quat GetRotation() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetRotation();
		}

		VT_NODISCARD glm::vec3 GetScale() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetScale();
		}

		VT_NODISCARD glm::vec3 GetLocalPosition() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetLocalPosition();
		}

		VT_NODISCARD glm::quat GetLocalRotation() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetLocalRotation();
		}

		VT_NODISCARD glm::vec3 GetLocalScale() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetLocalScale();
		}

		VT_NODISCARD glm::vec3 GetForward() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetForward();
		}

		VT_NODISCARD glm::vec3 GetRight() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetRight();
		}

		VT_NODISCARD glm::vec3 GetUp() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetUp();
		}

		VT_NODISCARD glm::vec3 GetLocalForward() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetLocalForward();
		}

		VT_NODISCARD glm::vec3 GetLocalRight() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetLocalRight();
		}

		VT_NODISCARD glm::vec3 GetLocalUp() const requires ComponentIsSpecifiedInAccessor<Volt::TransformComponent, ComponentTupleRaw>
		{
			return m_entity.GetLocalUp();
		}

		VT_NODISCARD Volt::EntityID GetID() const requires ComponentIsSpecifiedInAccessor<Volt::IDComponent, ComponentTupleRaw>
		{
			return m_entity.GetID();
		}

		VT_NODISCARD VT_INLINE entt::entity GetHandle() const { return m_entity.GetHandle(); }
		VT_NODISCARD Volt::RenderScene* GetRenderScene() const
		{
			return m_entity.GetSceneReference()->GetRenderScene();
		}

		template<std::size_t N>
		decltype(auto) get()
		{
			using ComponentType = std::tuple_element_t<N, ComponentTupleStructuredBindings>;
			return GetComponent<std::remove_reference_t<ComponentType>>();
		}

	private:
		Volt::Entity m_entity;
	};

	template<typename T>
	struct StorageForType
	{
		using Type = typename entt::storage_for<std::remove_reference_t<T>>::type;
	};

	template<typename Tuple, template<typename...> class TargetTemplate>
	struct ExpandAndApply;

	template<template<typename...> class TargetTemplate, typename... Ts>
	struct ExpandAndApply<std::tuple<Ts...>, TargetTemplate>
	{
		using Type = TargetTemplate<typename StorageForType<Ts>::Type...>;
	};

	template<typename... T>
	class ConstructComponents<Type::Query, T...>
	{
	public:
		using ComponentViewTuple = typename FilterComponents<IsNotWithout, false, T...>::Type;
		using ComponentViewExcludeTuple = typename FilterComponents<IsWithout, false, T...>::Type;
		using ComponentTuple = typename FilterComponents<IsNotWithoutOrWith, false, T...>::Type;

		using ViewType = entt::basic_view<typename ExpandAndApply<ComponentViewTuple, entt::get_t>::Type, typename ExpandAndApply<ComponentViewExcludeTuple, entt::exclude_t>::Type>;

		inline static constexpr Type ConstructType = ::ECS::Type::Query;

		ConstructComponents(ViewType view, Volt::EntityScene* entityScene)
			: m_view(view), m_entityScene(entityScene)
		{
		}

		constexpr ConstructComponents() = default;

		class Iterator
		{
		public:
			constexpr Iterator(ViewType::iterator it, ViewType& view, Volt::EntityScene* entityScene)
				: m_iterator(it), m_view(view), m_entityScene(entityScene)
			{
			}

			VT_INLINE constexpr ConstructComponents<Type::Entity, T...> operator*() const
			{
				return ConstructComponents<Type::Entity, T...>(Volt::Entity(*m_iterator, m_entityScene));
			}

			VT_INLINE constexpr Iterator& operator++()
			{
				++m_iterator;
				return *this;
			}

			VT_NODISCARD VT_INLINE constexpr const bool operator!=(const Iterator& other) const
			{
				return m_iterator != other.m_iterator;
			}

			VT_NODISCARD VT_INLINE constexpr const bool operator==(const Iterator& other) const
			{
				return m_iterator == other.m_iterator;
			}

		private:
			ViewType::iterator m_iterator;
			ViewType& m_view;
			Volt::EntityScene* m_entityScene;
		};

		VT_INLINE constexpr Iterator begin() { return Iterator(m_view.begin(), m_view, m_entityScene); }
		VT_INLINE constexpr Iterator end() { return Iterator(m_view.end(), m_view, m_entityScene); }

		VT_INLINE constexpr const Iterator begin() const { return Iterator(m_view.begin(), m_view, m_entityScene); }
		VT_INLINE constexpr const Iterator end() const { return Iterator(m_view.end(), m_view, m_entityScene); }

	private:
		mutable ViewType m_view;
		Volt::EntityScene* m_entityScene;
	};

	template<typename... T>
	struct ComponentAccess
	{
		template<typename ComponentTraits>
		using Read = ComponentAccess<SingleComponentAccess<ComponentTraits, AccessType::Read>, T...>;

		template<typename ComponentTraits>
		using Write = ComponentAccess<SingleComponentAccess<ComponentTraits, AccessType::Write>, T...>;

		template<typename ComponentTraits>
		using With = ComponentAccess<SingleComponentAccess<ComponentTraits, AccessType::With>, T...>;

		template<typename ComponentTraits>
		using Without = ComponentAccess<SingleComponentAccess<ComponentTraits, AccessType::Without>, T...>;

		template<typename ComponentTraits>
		using ReadIfExists = ComponentAccess<SingleComponentAccess<ComponentTraits, AccessType::ReadIfExists>, T...>;

		template<typename ComponentTraits>
		using WriteIfExists = ComponentAccess<SingleComponentAccess<ComponentTraits, AccessType::WriteIfExists>, T...>;

		template<Type type>
		using As = ConstructComponents<type, T...>;
	};

	class Access
	{
	public:
		template<typename T>
		using Read = ComponentAccess<SingleComponentAccess<T, AccessType::Read>>;

		template<typename T>
		using Write = ComponentAccess<SingleComponentAccess<T, AccessType::Write>>;

		template<typename T>
		using With = ComponentAccess<SingleComponentAccess<T, AccessType::With>>;

		template<typename T>
		using Without = ComponentAccess<SingleComponentAccess<T, AccessType::Without>>;

		template<typename T>
		using ReadIfExists = ComponentAccess<SingleComponentAccess<T, AccessType::ReadIfExists>>;

		template<typename T>
		using WriteIfExists = ComponentAccess<SingleComponentAccess<T, AccessType::WriteIfExists>>;
	};
}

namespace std
{
	template<typename... T>
	struct tuple_size<ECS::ConstructComponents<ECS::Type::Entity, T...>>
	: std::integral_constant<std::size_t, std::tuple_size_v<typename ECS::FilterComponents<ECS::IsGuaranteedAccessible, false, T...>::Type>> {};

	template<std::size_t N, typename... T>
	struct tuple_element<N, ECS::ConstructComponents<ECS::Type::Entity, T...>>
	{
		using TupleType = typename ECS::ConstructComponents<ECS::Type::Entity, T...>::ComponentTupleStructuredBindings;
		using type = std::tuple_element_t<N, TupleType>;
	};
}
