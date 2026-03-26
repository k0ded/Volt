#pragma once

#include "Config.h"
#include "ComponentReflection.h"

#include "EntitySystem/EntityScene.h"
#include "EntitySystem/Entity.h"

#include <CoreUtilities/Containers/Map.h>

#include <entt.hpp>

namespace Volt
{
	class Entity;
	class VTES_API ComponentRegistry
	{
	public:
		template<typename T> void RegisterComponent();
		template<typename T> void UnregisterComponent();

		template<typename T> void RegisterEnum();
		template<typename T> void UnregisterEnum();

		void ClearRegistry();

		const ICommonTypeDesc* GetTypeDescFromName(StringView name);
		const ICommonTypeDesc* GetTypeDescFromGUID(const VoltGUID& guid);
		StringView GetTypeNameFromGUID(const VoltGUID& guid);
		const VoltGUID GetGUIDFromTypeName(StringView typeName);

		inline const auto& GetRegistry() { return m_typeRegistry; }

		struct HelperFunctions
		{
			std::function<void(entt::registry&, entt::entity)> addComponent;
			std::function<void(entt::registry&, entt::entity)> removeComponent;
			std::function<bool(const entt::registry&, entt::entity)> hasComponent;
			std::function<void* (entt::registry&, entt::entity)> getComponent;
			std::function<void(entt::registry&)> setupOnCreate;
			std::function<void(entt::registry&)> setupOnDestroy;
		};

		class Helpers
		{
		public:
			VTES_API static void AddComponentWithGUID(const VoltGUID& guid, entt::registry& registry, entt::entity entity);
			VTES_API static void RemoveComponentWithGUID(const VoltGUID& guid, entt::registry& registry, entt::entity entity);
			VTES_API static const bool HasComponentWithGUID(const VoltGUID& guid, const entt::registry& registry, entt::entity entity);
			VTES_API static void* GetComponentWithGUID(const VoltGUID& guid, entt::registry& registry, entt::entity entity);
			VTES_API static void SetupComponentCallbacks(entt::registry& registry);
		private:
			Helpers() = default;
		};

		static ComponentRegistry& Get();

	private:
		friend class Helpers;

		template<typename T>
		static void OnConstructComponent(entt::registry& registry, entt::entity entity);

		template<typename T>
		static void OnDestructComponent(entt::registry& registry, entt::entity entity);

		Map<VoltGUID, HelperFunctions> m_componentHelperFunctions;
		Map<VoltGUID, const ICommonTypeDesc*> m_typeRegistry;
		Map<StringView, VoltGUID> m_typeNameToGUIDMap;
		Map<VoltGUID, StringView> m_guidToTypeNameMap;
	};

	template<typename T>
	inline void ComponentRegistry::RegisterComponent()
	{
		static_assert(IsReflectedType<T>());

		const auto guid = GetTypeGUID<T>();
		VT_ENSURE(!m_typeRegistry.contains(guid));

		constexpr std::string_view tempName = entt::type_name<T>();
		constexpr StringView name(tempName.data(), tempName.size());

		m_typeRegistry[guid] = GetTypeDesc<T>();
		m_typeNameToGUIDMap[name] = guid;
		m_guidToTypeNameMap[guid] = name;

		auto& helpers = m_componentHelperFunctions[guid];
		helpers.addComponent = [](entt::registry& registry, entt::entity entity)
		{
			registry.emplace<T>(entity);
		};

		helpers.removeComponent = [](entt::registry& registry, entt::entity entity)
		{
			registry.remove<T>(entity);
		};

		helpers.hasComponent = [](const entt::registry& registry, entt::entity entity)
		{
			return registry.any_of<T>(entity);
		};

		helpers.getComponent = [](entt::registry& registry, entt::entity entity) -> void*
		{
			if (registry.any_of<T>(entity))
			{
				return reinterpret_cast<void*>(&registry.get<T>(entity));
			}
			else
			{
				return nullptr;
			}
		};

		helpers.setupOnCreate = [](entt::registry& registry)
		{
			registry.on_construct<T>().template connect<&ComponentRegistry::OnConstructComponent<T>>();
		};

		helpers.setupOnDestroy = [](entt::registry& registry)
		{
			registry.on_destroy<T>().template connect<&ComponentRegistry::OnDestructComponent<T>>();
		};
	}

	template<typename T>
	void ComponentRegistry::UnregisterComponent()
	{
		const auto guid = GetTypeGUID<T>();

		// #Note_Ivar: The registry may already have been destroyed due to
		// DLL ordering.
		if (m_typeRegistry.empty())
		{
			return;
		}

		if (VT_CHECK(m_guidToTypeNameMap.contains(guid)))
		{
			m_typeNameToGUIDMap.erase(m_guidToTypeNameMap.at(guid));
			m_guidToTypeNameMap.erase(guid);
			m_typeRegistry.erase(guid);
			m_componentHelperFunctions.erase(guid);
		}
	}

	template<typename T>
	inline void ComponentRegistry::RegisterEnum()
	{
		static_assert(IsReflectedType<T>() && std::is_enum<T>::value);

		const auto guid = GetTypeGUID<T>();
		VT_ENSURE(!m_typeRegistry.contains(guid));

		constexpr std::string_view tempName = entt::type_name<T>();
		constexpr StringView name(tempName.data(), tempName.size());

		m_typeRegistry[guid] = GetTypeDesc<T>();
		m_typeNameToGUIDMap[name] = guid;
		m_guidToTypeNameMap[guid] = name;
	}

	template<typename T>
	void ComponentRegistry::UnregisterEnum()
	{
		const auto guid = GetTypeGUID<T>();

		// #Note_Ivar: The registry may already have been destroyed due to
		// DLL ordering.
		if (m_typeRegistry.empty())
		{
			return;
		}

		if (VT_CHECK(m_guidToTypeNameMap.contains(guid)))
		{
			m_typeNameToGUIDMap.erase(m_guidToTypeNameMap.at(guid));
			m_guidToTypeNameMap.erase(guid);
			m_typeRegistry.erase(guid);
		}
	}

	template<typename T>
	inline void ComponentRegistry::OnConstructComponent(entt::registry& registry, entt::entity entity)
	{
		//nothing to do here right now...
	}

	template<typename T>
	inline void ComponentRegistry::OnDestructComponent(entt::registry& registry, entt::entity entity)
	{
		const auto* typeDesc = GetTypeDesc<T>();
		const IComponentTypeDesc* compDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);

		EntityScene* entityScene = reinterpret_cast<EntityScene*>(registry.get_user_data());
		VT_ENSURE(entityScene);

		compDesc->OnDestroy(entityScene->GetEntityFromHandle(entity));
	}
}

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_COMPONENT(compType) \
	class ComponentRegistrar_##compType \
	{ \
	public: \
		VT_INLINE ComponentRegistrar_##compType() \
		{ \
			::Volt::ComponentRegistry::Get().RegisterComponent<compType>(); \
		} \
		VT_INLINE ~ComponentRegistrar_##compType() \
		{ \
			::Volt::ComponentRegistry::Get().UnregisterComponent<compType>(); \
		} \
	} g_componentRegistrar_##compType

#define VT_REGISTER_ENUM(compType) \
	class EnumRegistrar_##compType \
	{ \
	public: \
		VT_INLINE EnumRegistrar_##compType() \
		{ \
			::Volt::ComponentRegistry::Get().RegisterEnum<compType>(); \
		} \
		VT_INLINE ~EnumRegistrar_##compType() \
		{ \
			::Volt::ComponentRegistry::Get().UnregisterEnum<compType>(); \
		} \
	} g_enumRegistrar_##compType
