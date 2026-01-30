#include "espch.h"

#include "EntitySystem/Entity.h"
#include "EntitySystem/ComponentRegistry.h"
#include "EntitySystem/Scripting/CoreComponents.h"

#include <CoreUtilities/StringUtility.h>
#include <CoreUtilities/Math/TQS.h>
#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	Entity::Entity()
	{}

	Entity::Entity(entt::entity entityHandle, EntityScene* scene)
		: m_handle(entityHandle), m_sceneReference(scene)
	{}

	Entity::Entity(entt::entity entityHandle, const EntityScene* scene)
		: m_handle(entityHandle), m_sceneReference(const_cast<EntityScene*>(scene))
	{}

	Entity::Entity(entt::entity entityHandle, EntityScene& scene)
		: m_handle(entityHandle), m_sceneReference(&scene)
	{}

	Entity::Entity(entt::entity entityHandle, const EntityScene& scene)
		: m_handle(entityHandle), m_sceneReference(const_cast<EntityScene*>(&scene))
	{}

	Entity::~Entity()
	{}

	void Entity::SetTag(const std::string& tag)
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TagComponent>());

		GetComponent<TagComponent>().tag = tag;
	}

	void Entity::SetPosition(const glm::vec3& position)
	{
		VT_ENSURE(IsValid());

		Entity parent = GetParent();
		TQS parentTransform;

		if (parent)
		{
			parentTransform = m_sceneReference->GetEntityWorldTQS(parent);
		}

		const glm::vec3 translatedPosition = position - parentTransform.translation;
		const glm::vec3 invertedScale = 1.f / parentTransform.scale;
		const glm::vec3 rotatedPoint = glm::conjugate(parentTransform.rotation) * translatedPosition;

		const glm::vec3 localPoint = rotatedPoint * invertedScale;
		SetLocalPosition(localPoint);
	}

	void Entity::SetRotation(const glm::quat& rotation)
	{
		VT_ENTITY_VALIDATE(IsValid());

		Entity parent = GetParent();
		TQS parentTransform;

		if (parent)
		{
			parentTransform = m_sceneReference->GetEntityWorldTQS(parent);
		}

		const glm::quat localRotation = glm::conjugate(parentTransform.rotation) * rotation;
		SetLocalRotation(localRotation);
	}

	void Entity::SetScale(const glm::vec3& scale)
	{
		VT_ENTITY_VALIDATE(IsValid());

		Entity parent = GetParent();
		TQS parentTransform{};

		if (parent)
		{
			parentTransform = m_sceneReference->GetEntityWorldTQS(parent);
		}

		const glm::vec3 inverseScale = 1.f / parentTransform.scale;
		const glm::vec3 localScale = scale * inverseScale;

		SetLocalScale(localScale);
	}

	void Entity::SetLocalPosition(const glm::vec3& position)
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TransformComponent>());

		GetComponent<TransformComponent>().position = position;
		m_sceneReference->InvalidateEntityTransform(GetID());
	}

	void Entity::SetLocalRotation(const glm::quat& rotation)
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TransformComponent>());

		GetComponent<TransformComponent>().rotation = rotation;
		m_sceneReference->InvalidateEntityTransform(GetID());
	}

	void Entity::SetLocalScale(const glm::vec3& scale)
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TransformComponent>());

		GetComponent<TransformComponent>().scale = scale;
		m_sceneReference->InvalidateEntityTransform(GetID());
	}

	void Entity::SetParent(Entity parentEntity)
	{
		ParentEntity(parentEntity, *this);
	}

	void Entity::AddChild(Entity childEntity)
	{
		ParentEntity(*this, childEntity);
	}

	void Entity::RemoveChild(Entity entity)
	{
		entity.UnparentEntity();
	}

	void Entity::ClearChildren()
	{
		auto& registry = m_sceneReference->GetRegistry();

		assert(registry.any_of<RelationshipComponent>(m_handle) && "Entity_DEPRECATED must have relationship component!");

		auto& children = registry.get<RelationshipComponent>(m_handle).children;

		for (auto& childId : children)
		{
			auto child = m_sceneReference->GetEntityFromID(childId);

			if (child.GetParent().GetID() == GetID())
			{
				child.GetComponent<RelationshipComponent>().parent = NullID();
			}
		}

		children.clear();
	}

	glm::vec3 Entity::GetForward() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		return glm::rotate(GetRotation(), glm::vec3{ 0.f, 0.f, 1.f });
	}

	glm::vec3 Entity::GetRight() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		return glm::rotate(GetRotation(), glm::vec3{ 1.f, 0.f, 0.f });
	}

	glm::vec3 Entity::GetUp() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		return glm::rotate(GetRotation(), glm::vec3{ 0.f, 1.f, 0.f });
	}

	glm::vec3 Entity::GetLocalForward() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().GetForward();
	}

	glm::vec3 Entity::GetLocalRight() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().GetRight();
	}

	glm::vec3 Entity::GetLocalUp() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().GetUp();
	}

	glm::vec3 Entity::GetPosition() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		const auto tqs = m_sceneReference->GetEntityWorldTQS(*this);
		return tqs.translation;
	}

	glm::quat Entity::GetRotation() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		const auto tqs = m_sceneReference->GetEntityWorldTQS(*this);
		return tqs.rotation;
	}

	glm::vec3 Entity::GetScale() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		const auto tqs = m_sceneReference->GetEntityWorldTQS(*this);
		return tqs.scale;
	}

	const glm::vec3& Entity::GetLocalPosition() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().position;
	}

	const glm::quat& Entity::GetLocalRotation() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().rotation;
	}

	const glm::vec3& Entity::GetLocalScale() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().scale;
	}

	const std::string& Entity::GetTag() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<TagComponent>());

		return GetComponent<TagComponent>().tag;
	}

	glm::mat4 Entity::GetTransform() const
	{
		const TQS entityWorldTQS = m_sceneReference->GetEntityWorldTQS(*this);

		const glm::mat4 transform = glm::translate(glm::mat4{ 1.f }, entityWorldTQS.translation)
			* glm::mat4_cast(entityWorldTQS.rotation)
			* glm::scale(glm::mat4{ 1.f }, entityWorldTQS.scale);

		return transform;
	}

	glm::mat4 Entity::GetLocalTransform() const
	{
		VT_ASSERT_MSG(HasComponent<TransformComponent>(), "Entity must have transform component!");
		return  GetComponent<TransformComponent>().GetTransform();
	}

	bool Entity::HasParent() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		return GetParent().IsValid();
	}

	Entity Entity::GetParent() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<RelationshipComponent>());

		const auto& relationshipComponent = GetComponent<RelationshipComponent>();
		return m_sceneReference->GetEntityFromID(relationshipComponent.parent);
	}

	Vector<Entity> Entity::GetChildren() const
	{
		if (!HasComponent<RelationshipComponent>())
		{
			return {};
		}

		const auto& children = GetComponent<RelationshipComponent>().children;

		Vector<Entity> result{};
		for (const auto& id : children)
		{
			auto entity = m_sceneReference->GetEntityFromID(id);
			if (entity != Entity::Null())
			{
				result.emplace_back(entity);
			}
		}

		return result;
	}

	EntityID Entity::GetID() const
	{
		VT_ENTITY_VALIDATE(IsValid());
		VT_ENTITY_VALIDATE(HasComponent<IDComponent>());

		return GetComponent<IDComponent>().id;
	}

	const std::string Entity::ToString() const
	{
		return std::to_string(static_cast<uint32_t>(GetComponent<IDComponent>().id));
	}

	bool Entity::IsVisible() const
	{
		VT_ASSERT_MSG(HasComponent<TransformComponent>(), "Entity must have transform component!");
		return GetComponent<TransformComponent>().visible;
	}

	bool Entity::IsLocked() const
	{
		VT_ASSERT_MSG(HasComponent<TransformComponent>(), "Entity must have transform component!");
		return GetComponent<TransformComponent>().locked;
	}

	bool Entity::IsDistantParentOf(Entity potentialChild) const
	{
		if (!HasComponent<RelationshipComponent>())
		{
			return false;
		}
		Entity checking = potentialChild;
		while (checking.IsValid())
		{
			if (checking.GetParent() == *this)
			{
				return true;
			}
			checking = checking.GetParent();
		}

		return false;
	}

	bool Entity::IsDistantChildOf(Entity potentialParent) const
	{
		return potentialParent.IsDistantParentOf(*this);
	}

	void Entity::RemoveComponent(const VoltGUID& guid)
	{
		VT_ENTITY_VALIDATE(IsValid());

		ComponentRegistry::Helpers::RemoveComponentWithGUID(guid, m_sceneReference->GetRegistry(), m_handle);
	}

	bool Entity::HasComponent(std::string_view componentName) const
	{
		VT_ENTITY_VALIDATE(IsValid());

		//const std::string lowerCompName = ::Utility::ToLower(std::string(componentName));
		const ICommonTypeDesc* compType = ComponentRegistry::Get().GetTypeDescFromName(componentName);
		return ComponentRegistry::Helpers::HasComponentWithGUID(compType->GetGUID(), m_sceneReference->GetRegistry(), m_handle);
	}

	bool Entity::HasComponent(const VoltGUID& componentGUID) const
	{
		return ComponentRegistry::Helpers::HasComponentWithGUID(componentGUID, m_sceneReference->GetRegistry(), m_handle);
	}

	void Entity::InitializeComponents()
	{
		VT_ENTITY_VALIDATE(IsValid());

		for (auto&& curr : m_sceneReference->GetRegistry().storage())
		{
			auto& storage = curr.second;

			if (!storage.contains(m_handle))
			{
				continue;
			}

			const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(ComponentRegistry::Get().GetTypeDescFromName(storage.type().name()));
			if (!componentDesc)
			{
				continue;
			}
			componentDesc->OnInitialize(*this);
		}
	}

	Entity Entity::Null()
	{
		return {};
	}

	void Entity::ParentEntity(Entity parent, Entity child)
	{
		if (!parent.IsValid() || !child.IsValid() || parent.GetHandle() == child.GetHandle())
		{
			return;
		}

		// Check that it's not in the child chain
		bool isChild = false;
		IsRecursiveChildOf(parent, child, isChild);
		if (isChild)
		{
			return;
		}

		if (child.GetParent())
		{
			child.UnparentEntity();
		}

		auto& childChildren = child.GetComponent<RelationshipComponent>().children;

		if (auto it = std::find(childChildren.begin(), childChildren.end(), parent.GetID()) != childChildren.end())
		{
			return;
		}

		child.GetComponent<RelationshipComponent>().parent = parent.GetID();
		parent.GetComponent<RelationshipComponent>().children.emplace_back(child.GetID());

		child.ConvertToLocalSpace();
	}

	void Entity::UnparentEntity()
	{
		if (!IsValid()) { return; }

		auto parent = GetParent();
		if (!parent.IsValid())
		{
			return;
		}

		auto& children = parent.GetComponent<RelationshipComponent>().children;

		auto it = std::find(children.begin(), children.end(), GetID());
		if (it != children.end())
		{
			children.erase(it);
		}

		//we need to convert to world space before removing the parent because it takes the parent transform into account
		ConvertToWorldSpace();
		GetComponent<RelationshipComponent>().parent = Entity::NullID();

		//we have to invalidate the transform here even though ConvertToWorldSpace already does it since it takes the parent into account
		m_sceneReference->InvalidateEntityTransform(GetID());
	}

	void Entity::IsRecursiveChildOf(Entity parent, Entity currentEntity, bool& outChild)
	{
		if (currentEntity.HasComponent<RelationshipComponent>())
		{
			auto& relComp = currentEntity.GetComponent<RelationshipComponent>();
			for (const auto& childId : relComp.children)
			{
				Entity child = m_sceneReference->GetEntityFromID(childId);
				outChild |= (parent.GetID() == childId) && (childId != parent.GetID());

				IsRecursiveChildOf(parent, child, outChild);
			}
		}
	}

	void Entity::ConvertToWorldSpace()
	{
		Entity parent = GetParent();

		if (!parent)
		{
			return;
		}

		auto& transform = GetComponent<TransformComponent>();

		const glm::mat4 transformMatrix = GetTransform();

		glm::vec3 r;
		Math::Decompose(transformMatrix, transform.position, r, transform.scale);

		transform.rotation = glm::quat{ r };

		m_sceneReference->InvalidateEntityTransform(GetID());
	}

	void Entity::ConvertToLocalSpace()
	{
		Entity parent = GetParent();

		if (!parent)
		{
			return;
		}

		auto& transform = GetComponent<TransformComponent>();
		const glm::mat4 parentTransform = parent.GetTransform();
		const glm::mat4 localTransform = glm::inverse(parentTransform) * transform.GetTransform();

		glm::vec3 r;
		Math::Decompose(localTransform, transform.position, r, transform.scale);
		transform.rotation = glm::quat{ r };

		m_sceneReference->InvalidateEntityTransform(GetID());
	}
}
