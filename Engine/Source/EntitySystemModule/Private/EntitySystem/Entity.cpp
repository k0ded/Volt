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

	Entity::Entity(entt::entity entityHandle, EntityScene & scene)
		: m_handle(entityHandle), m_sceneReference(&scene)
	{}
	
	Entity::Entity(entt::entity entityHandle, const EntityScene& scene)
		: m_handle(entityHandle), m_sceneReference(const_cast<EntityScene*>(&scene))
	{}

	Entity::~Entity()
	{}

	void Entity::SetTag(const std::string& tag)
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TagComponent>());

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
		VT_ENSURE(IsValid());

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
		VT_ENSURE(IsValid());

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
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TransformComponent>());

		GetComponent<TransformComponent>().position = position;
		m_sceneReference->InvalidateEntityTransform(GetID());
	}

	void Entity::SetLocalRotation(const glm::quat& rotation)
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TransformComponent>());

		GetComponent<TransformComponent>().rotation = rotation;
		m_sceneReference->InvalidateEntityTransform(GetID());
	}

	void Entity::SetLocalScale(const glm::vec3& scale)
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TransformComponent>());

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
		VT_ENSURE(IsValid());

		return glm::rotate(GetRotation(), glm::vec3{ 0.f, 0.f, 1.f });
	}

	glm::vec3 Entity::GetRight() const
	{
		VT_ENSURE(IsValid());

		return glm::rotate(GetRotation(), glm::vec3{ 1.f, 0.f, 0.f });
	}

	glm::vec3 Entity::GetUp() const
	{
		VT_ENSURE(IsValid());

		return glm::rotate(GetRotation(), glm::vec3{ 0.f, 1.f, 0.f });
	}

	glm::vec3 Entity::GetLocalForward() const
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().GetForward();
	}

	glm::vec3 Entity::GetLocalRight() const
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().GetRight();
	}

	glm::vec3 Entity::GetLocalUp() const
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().GetUp();
	}

	glm::vec3 Entity::GetPosition() const
	{
		VT_ENSURE(IsValid());
		const auto tqs = m_sceneReference->GetEntityWorldTQS(*this);
		return tqs.translation;
	}

	glm::quat Entity::GetRotation() const
	{
		VT_ENSURE(IsValid());
		const auto tqs = m_sceneReference->GetEntityWorldTQS(*this);
		return tqs.rotation;
	}

	glm::vec3 Entity::GetScale() const
	{
		VT_ENSURE(IsValid());
		const auto tqs = m_sceneReference->GetEntityWorldTQS(*this);
		return tqs.scale;
	}

	const glm::vec3& Entity::GetLocalPosition() const
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().position;
	}

	const glm::quat& Entity::GetLocalRotation() const
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().rotation;
	}

	const glm::vec3& Entity::GetLocalScale() const
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TransformComponent>());

		return GetComponent<TransformComponent>().scale;
	}

	const std::string& Entity::GetTag() const
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<TagComponent>());

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
		VT_ENSURE(IsValid());
		return GetParent().IsValid();
	}

	Entity Entity::GetParent() const
	{
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<RelationshipComponent>());

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
		VT_ENSURE(IsValid());
		VT_ENSURE(HasComponent<IDComponent>());

		return GetComponent<IDComponent>().id;
	}

	const std::string Entity::ToString() const
	{
		return std::to_string(static_cast<uint32_t>(GetComponent<IDComponent>().id));
	}

	bool Entity::IsVisible() const
	{
		assert(HasComponent<TransformComponent>() && "Entity must have transform component!");
		return GetComponent<TransformComponent>().visible;
	}

	bool Entity::IsLocked() const
	{
		assert(HasComponent<TransformComponent>() && "Entity must have transform component!");
		return GetComponent<TransformComponent>().locked;
	}

	void Entity::RemoveComponent(const VoltGUID& guid)
	{
		VT_ENSURE(IsValid());

		ComponentRegistry::Helpers::RemoveComponentWithGUID(guid, m_sceneReference->GetRegistry(), m_handle);
	}

	bool Entity::HasComponent(std::string_view componentName) const
	{
		VT_ENSURE(IsValid());

		const std::string lowerCompName = ::Utility::ToLower(std::string(componentName));
		const ICommonTypeDesc* compType = GetComponentRegistry().GetTypeDescFromName(lowerCompName);
		return ComponentRegistry::Helpers::HasComponentWithGUID(compType->GetGUID(), m_sceneReference->GetRegistry(), m_handle);
	}

	bool Entity::HasComponent(const VoltGUID& componentGUID) const
	{
		return ComponentRegistry::Helpers::HasComponentWithGUID(componentGUID, m_sceneReference->GetRegistry(), m_handle);
	}

	void Entity::Copy(Entity srcEntity, Entity dstEntity, std::set<VoltGUID> componentsToSkip)
	{
		auto srcScene = dstEntity.GetSceneReference();
		auto& srcRegistry = srcScene->GetRegistry();

		auto dstScene = srcEntity.GetSceneReference();
		auto& dstRegistry = dstScene->GetRegistry();

		for (auto&& curr : srcRegistry.storage())
		{
			auto& storage = curr.second;

			if (!storage.contains(srcEntity.GetHandle()))
			{
				continue;
			}

			const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(GetComponentRegistry().GetTypeDescFromName(storage.type().name()));
			if (!componentDesc)
			{
				continue;
			}

			if (componentDesc->GetValueType() != ValueType::Component)
			{
				continue;
			}

			if (!ComponentRegistry::Helpers::HasComponentWithGUID(componentDesc->GetGUID(), dstRegistry, dstEntity.GetHandle()))
			{
				ComponentRegistry::Helpers::AddComponentWithGUID(componentDesc->GetGUID(), dstRegistry, dstEntity.GetHandle());
			}

			void* voidCompPtr = Volt::ComponentRegistry::Helpers::GetComponentWithGUID(componentDesc->GetGUID(), dstRegistry, dstEntity.GetHandle());
			uint8_t* componentData = reinterpret_cast<uint8_t*>(voidCompPtr);

			if (componentsToSkip.contains(componentDesc->GetGUID()))
			{
				continue;
			}
			CopyComponent(reinterpret_cast<const uint8_t*>(storage.get(srcEntity.GetHandle())), componentData, 0, componentDesc, dstEntity);
		}
	}

	Entity Entity::Duplicate(Entity srcEntity, EntityScene* targetScene, Entity parent, std::set<VoltGUID> componentsToSkip)
	{
		auto scene = targetScene ? targetScene : srcEntity.GetSceneReference();

		Entity newEntity = scene->CreateEntity();

		auto allSkipComponents = CreateSkipComponentOnCopySet<IDComponent, RelationshipComponent>();
		allSkipComponents.insert(componentsToSkip.begin(), componentsToSkip.end());
		Copy(srcEntity, newEntity, allSkipComponents);

		Vector<EntityID> newChildren;

		for (const auto& child : srcEntity.GetChildren())
		{
			newChildren.emplace_back(Duplicate(child, targetScene, newEntity).GetID());
		}

		newEntity.GetComponent<RelationshipComponent>().children = newChildren;
		newEntity.GetComponent<RelationshipComponent>().parent = parent ? parent.GetID() : Entity::NullID();

		scene->InvalidateEntityTransform(newEntity.GetID());

		return newEntity;
	}
	void Entity::CopyComponent(const uint8_t* srcData, uint8_t* dstData, const size_t offset, const IComponentTypeDesc* compDesc, Entity dstEntity)
	{
		for (const auto& member : compDesc->GetMembers())
		{
			if ((member.flags & ComponentMemberFlag::NoCopy) != ComponentMemberFlag::None)
			{
				continue;
			}

			if (member.typeDesc != nullptr)
			{
				switch (member.typeDesc->GetValueType())
				{
					case ValueType::Component:
					{
						const IComponentTypeDesc* memberCompDesc = reinterpret_cast<const IComponentTypeDesc*>(member.typeDesc);
						CopyComponent(srcData, dstData, offset + member.offset, memberCompDesc, dstEntity);
						break;
					}

					case ValueType::Enum:
						*reinterpret_cast<int32_t*>(&dstData[offset + member.offset]) = *(reinterpret_cast<const int32_t*>(&srcData[offset + member.offset]));
						break;

					case ValueType::Array:
						member.copyFunction(&dstData[offset + member.offset], &srcData[offset + member.offset]);
						break;
				}
			}
			else
			{
				member.copyFunction(&dstData[offset + member.offset], &srcData[offset + member.offset]);
			}
		}

		compDesc->OnComponentCopied(dstEntity);
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
