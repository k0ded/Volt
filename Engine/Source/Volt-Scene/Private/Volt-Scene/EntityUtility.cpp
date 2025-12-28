#include "vspch.h"

#include "Volt-Scene/EntityUtility.h"
#include "Volt-Scene/Scene.h"

#include <EntitySystem/ComponentRegistry.h>

#include <CoreUtilities/FileIO/YAMLFileStreamWriter.h>
#include <CoreUtilities/FileIO/YAMLFileStreamReader.h>
#include <EntitySystem/Scripting/CoreComponents.h>

namespace Volt
{
	void CopyEntity(Entity srcEntity, Entity dstEntity, std::set<VoltGUID> componentsToSkip)
	{
		auto srcScene = srcEntity.GetSceneReference();
		auto& srcRegistry = srcScene->GetRegistry();

		auto dstScene = dstEntity.GetSceneReference();
		auto& dstRegistry = dstScene->GetRegistry();

		for (auto&& curr : srcRegistry.storage())
		{
			auto& storage = curr.second;

			if (!storage.contains(srcEntity.GetHandle()))
			{
				continue;
			}

			const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(ComponentRegistry::Get().GetTypeDescFromName(storage.type().name()));
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

	Entity DuplicateEntity(Entity srcEntity, Scene& targetScene, Entity parent, std::set<VoltGUID> componentsToSkip)
	{
		Entity newEntity = targetScene.CreateEntity();

		auto allSkipComponents = CreateSkipComponentOnCopySet<IDComponent, RelationshipComponent>();
		allSkipComponents.insert(componentsToSkip.begin(), componentsToSkip.end());
		CopyEntity(srcEntity, newEntity, allSkipComponents);

		Vector<EntityID> newChildren;

		for (const auto& child : srcEntity.GetChildren())
		{
			newChildren.emplace_back(DuplicateEntity(child, targetScene, newEntity).GetID());
		}

		newEntity.GetComponent<RelationshipComponent>().children = newChildren;
		newEntity.GetComponent<RelationshipComponent>().parent = parent ? parent.GetID() : Entity::NullID();

		//initialize all components after the entity has spawned
		newEntity.InitializeComponents();

		targetScene.InvalidateEntityTransform(newEntity.GetID());

		return newEntity;
	}

	void CopyComponent(const uint8_t* srcData, uint8_t* dstData, const size_t offset, const IComponentTypeDesc* compDesc, Entity dstEntity)
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
	}
}
