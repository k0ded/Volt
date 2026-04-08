#pragma once

#include "EntitySystem/EntityScene.h" 
#include "EntitySystem/Config.h"
#include "EntitySystem/EntityID.h"
#include "EntitySystem/ComponentReflection.h"

#include <CoreUtilities/VoltGUID.h>

#include <entt.hpp>
#include <glm/fwd.hpp>

namespace Volt
{
	class EntityScene;
	class EntityID;

	class VTES_API Entity
	{
	public:
		VT_NODISCARD static Entity Null();
		VT_NODISCARD constexpr static EntityID NullID() { return EntityID(0); }
	public:
		Entity();
		Entity(entt::entity entityHandle, EntityScene* scene);
		Entity(entt::entity entityHandle, const EntityScene* scene);
		Entity(entt::entity entityHandle, EntityScene& scene);
		Entity(entt::entity entityHandle, const EntityScene& scene);
		~Entity();

		//setters
		void SetTag(const String& tag);

		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::quat& rotation);
		void SetScale(const glm::vec3& scale);

		void SetLocalPosition(const glm::vec3& position);
		void SetLocalRotation(const glm::quat& rotation);
		void SetLocalScale(const glm::vec3& scale);

		void SetParent(Entity parentEntity);
		void UnparentEntity();
		void AddChild(Entity childEntity);
		void RemoveChild(Entity entity);
		void ClearChildren();

		//getters
		VT_NODISCARD const String& GetTag() const;

		VT_NODISCARD glm::mat4 GetTransform() const;
		VT_NODISCARD glm::mat4 GetLocalTransform() const;
		VT_NODISCARD TQS GetTransformTQS() const;

		VT_NODISCARD glm::vec3 GetPosition() const;
		VT_NODISCARD glm::quat GetRotation() const;
		VT_NODISCARD glm::vec3 GetScale() const;

		VT_NODISCARD const glm::vec3& GetLocalPosition() const;
		VT_NODISCARD const glm::quat& GetLocalRotation() const;
		VT_NODISCARD const glm::vec3& GetLocalScale() const;

		VT_NODISCARD glm::vec3 GetForward() const;
		VT_NODISCARD glm::vec3 GetRight() const;
		VT_NODISCARD glm::vec3 GetUp() const;

		VT_NODISCARD glm::vec3 GetLocalForward() const;
		VT_NODISCARD glm::vec3 GetLocalRight() const;
		VT_NODISCARD glm::vec3 GetLocalUp() const;

		VT_NODISCARD Entity GetParent() const;
		VT_NODISCARD bool HasParent() const;

		VT_NODISCARD Vector<Entity> GetChildren() const;

		VT_NODISCARD EntityID GetID() const;
		VT_NODISCARD VT_INLINE entt::entity GetHandle() const { return m_handle; }

		//utility
		VT_NODISCARD const String ToString() const;
		VT_NODISCARD bool IsValid() const { return m_handle != entt::null && m_sceneReference != nullptr && m_sceneReference->GetRegistry().valid(m_handle); }
		VT_NODISCARD bool IsVisible() const;
		VT_NODISCARD bool IsLocked() const;
		//returns true if given entity has this entity as a parent or parent's parent or so on
		VT_NODISCARD bool IsDistantParentOf(Entity potentialChild) const;
		//returns true if given entity has this entity as a child or child's child or so on
		VT_NODISCARD bool IsDistantChildOf(Entity potentialParent) const;

		VT_NODISCARD VT_INLINE bool operator==(const Entity& entity) const { return m_handle == entity.m_handle && m_sceneReference == entity.m_sceneReference; }
		VT_NODISCARD VT_INLINE bool operator!() const { return !IsValid(); }
		VT_NODISCARD VT_INLINE explicit operator bool() const { return IsValid(); }
		VT_NODISCARD VT_INLINE explicit operator String() const { return ToString(); }
		VT_NODISCARD VT_INLINE operator entt::entity() const { return m_handle; }
		VT_NODISCARD VT_INLINE operator uint32_t() const { return static_cast<uint32_t>(m_handle); }

		//component handling
		template<typename T> VT_NODISCARD T& GetComponent();
		template<typename T> VT_NODISCARD const T& GetComponent() const;
		template<typename T> VT_NODISCARD bool HasComponent() const;
		template<typename T, typename... Args> T& AddComponent(Args&&... args);
		template<typename T> void RemoveComponent();
		void RemoveComponent(const VoltGUID& guid);
		VT_NODISCARD bool HasComponent(StringView componentName) const;
		VT_NODISCARD bool HasComponent(const VoltGUID& componentGUID) const;

		//this should only be called when constructing an entity without using the AddComponent helper as is initializes the added components as you add them
		void InitializeComponents();

		// #TODO_Ivar: Probably shouldn't expose this
		VT_NODISCARD VT_INLINE EntityScene* GetSceneReference() const { return m_sceneReference; }
	private:
		void ParentEntity(Entity parent, Entity child);
		void IsRecursiveChildOf(Entity mainParent, Entity currentEntity, bool& outChild);

		void ConvertToWorldSpace();
		void ConvertToLocalSpace();

		EntityScene* m_sceneReference = nullptr;
		entt::entity m_handle = entt::null;
	};

	template<typename T>
	inline T& Entity::GetComponent()
	{
		VT_ENTITY_VALIDATE(IsValid());

		auto& registry = m_sceneReference->GetRegistry();
		VT_ENSURE(registry.any_of<T>(m_handle));
		return registry.get<T>(m_handle);
	}

	template<typename T>
	inline const T& Entity::GetComponent() const
	{
		VT_ENTITY_VALIDATE(IsValid());

		auto& registry = m_sceneReference->GetRegistry();
		VT_ENSURE(registry.any_of<T>(m_handle));
		return registry.get<T>(m_handle);
	}

	template<typename T>
	inline bool Entity::HasComponent() const
	{
		VT_ENTITY_VALIDATE(IsValid());

		auto& registry = m_sceneReference->GetRegistry();
		return registry.any_of<T>(m_handle);
	}

	template<typename T, typename ...Args>
	inline T& Entity::AddComponent(Args && ...args)
	{
		VT_ENTITY_VALIDATE(IsValid());

		auto& registry = m_sceneReference->GetRegistry();
		VT_ENTITY_VALIDATE(!registry.any_of<T>(m_handle));
		T& createdComp = registry.emplace<T>(m_handle, std::forward<Args>(args)...);

		const ICommonTypeDesc* typeDesc = GetTypeDesc<T>();
		const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);
		componentDesc->OnInitialize(*this);

		return createdComp;
	}

	template<typename T>
	inline void Entity::RemoveComponent()
	{
		VT_ENTITY_VALIDATE(IsValid());

		auto& registry = m_sceneReference->GetRegistry();
		registry.remove<T>(m_handle);
	}
}
