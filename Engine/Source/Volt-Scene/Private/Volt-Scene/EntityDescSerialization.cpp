#include "vspch.h"

#include "Volt-Scene/EntityDescSerialization.h"

#include <EntitySystem/ComponentRegistry.h>

#include <CoreUtilities/Archive/MemoryArchive.h>

namespace Volt::EntityDescSerialization
{
	void SerializeClass(Archive& archive, const IComponentTypeDesc* componentDesc, const uint8_t* componentDataPtr)
	{
		Vector<MemberHeader, InlineAllocator<32>> memberHeaders;

		MemoryWriter componentMemberDataWriter;

		for (const auto& member : componentDesc->GetMembers())
		{
			if ((member.flags & ComponentMemberFlag::NoSerialize) != ComponentMemberFlag::None)
			{
				continue;
			}

			// Write the header.
			memberHeaders.emplace_back(std::string(member.name), member.size, componentMemberDataWriter.GetHeadLocation());

			const uint8_t* componentMemberDataPtr = componentDataPtr + member.offset;

			if (member.typeDesc != nullptr)
			{
				switch (member.typeDesc->GetValueType())
				{
					case ValueType::Component:
					{
						SerializeClass(componentMemberDataWriter, static_cast<const IComponentTypeDesc*>(member.typeDesc), componentMemberDataPtr);
						break;
					}

					case ValueType::Enum:
					{
						const IEnumTypeDesc* enumTypeDesc = static_cast<const IEnumTypeDesc*>(member.typeDesc);
						enumTypeDesc->Serialize(componentMemberDataWriter, const_cast<uint8_t*>(componentMemberDataPtr));
						break;
					}

					case ValueType::Array:
					{
						const IArrayTypeDesc* arrayTypeDesc = static_cast<const IArrayTypeDesc*>(member.typeDesc);
						arrayTypeDesc->Serialize(componentMemberDataWriter, const_cast<uint8_t*>(componentMemberDataPtr));
					}
				}
			}
			else
			{
				// Annoying const_cast....
				member.serializeFunction(componentMemberDataWriter, const_cast<uint8_t*>(componentMemberDataPtr));
			}
		}

		componentMemberDataWriter.Close();

		archive << memberHeaders;
		archive << componentMemberDataWriter;
	}

	void DeserializeClass(Archive& archive, const IComponentTypeDesc* componentDesc, uint8_t* componentDataPtr)
	{
		Vector<MemberHeader, InlineAllocator<32>> memberHeaders;
		MemoryReader componentMemberDataReader;

		archive << memberHeaders;
		archive << componentMemberDataReader;

		for (const auto& member : memberHeaders)
		{
			const ComponentMember* componentMember = componentDesc->FindMemberByName(member.name);
			if (!componentMember)
			{
				continue;
			}

			componentMemberDataReader.Seek(member.offset);

			uint8_t* componentMemberDataPtr = componentDataPtr + componentMember->offset;

			if (componentMember->typeDesc != nullptr)
			{
				switch (componentMember->typeDesc->GetValueType())
				{
					case ValueType::Component:
					{
						DeserializeClass(componentMemberDataReader, static_cast<const IComponentTypeDesc*>(componentMember->typeDesc), componentMemberDataPtr);
						break;
					}

					case ValueType::Enum:
					{
						const IEnumTypeDesc* enumTypeDesc = static_cast<const IEnumTypeDesc*>(componentMember->typeDesc);
						enumTypeDesc->Serialize(componentMemberDataReader, componentMemberDataPtr);
						break;
					}

					case ValueType::Array:
					{
						const IArrayTypeDesc* arrayTypeDesc = static_cast<const IArrayTypeDesc*>(componentMember->typeDesc);
						arrayTypeDesc->Serialize(componentMemberDataReader, componentMemberDataPtr);
						break;
					}
				}
			}
			else
			{
				if (componentMember->size != member.size)
				{
					continue;
				}

				componentMember->serializeFunction(componentMemberDataReader, componentMemberDataPtr);
			}
		}
	}

	void SerializeEntity(Archive& archive, Entity entity, AssetHandle ownerSceneAssetHandle)
	{
		// This should only be used for SAVING out entities.
		if (archive.IsLoading())
		{
			return;
		}

		EntityID entityId = entity.GetID();

		archive << entityId;
		archive << ownerSceneAssetHandle;

		entt::registry& registry = entity.GetSceneReference()->GetRegistry();

		Vector<ComponentHeader, InlineAllocator<32u>> componentHeaders;
		MemoryWriter componentDataWriter;

		for (auto&& curr : registry.storage())
		{
			auto& storage = curr.second;

			if (!storage.contains(entity.GetHandle()))
			{
				// Entity does not have this component, skip
				continue;
			}

			const IComponentTypeDesc* componentDesc = static_cast<const IComponentTypeDesc*>(GetComponentRegistry().GetTypeDescFromName(storage.type().name()));
			if (!componentDesc)
			{
				// Component isn't registered, skip
				continue;
			}

			// Write the header.
			componentHeaders.emplace_back(componentDesc->GetGUID(), componentDataWriter.GetHeadLocation());

			const uint8_t* componentPtr = reinterpret_cast<const uint8_t*>(storage.get(entity.GetHandle()));
			SerializeClass(componentDataWriter, componentDesc, componentPtr);
		}

		componentDataWriter.Close();

		archive << componentHeaders;
		archive << componentDataWriter;
	}

	void DeserializeEntity(Archive& archive, Entity entity)
	{
		VT_ENSURE(archive.IsLoading());

		SerializationData serializationData;
		DeserializeEntityData(archive, serializationData);

		VT_ENSURE(entity.GetID() == serializationData.entityId);

		auto& registry = entity.GetSceneReference()->GetRegistry();

		for (const ComponentHeader& componentHeader : serializationData.componentHeaders)
		{
			const IComponentTypeDesc* typeDesc = reinterpret_cast<const IComponentTypeDesc*>(GetComponentRegistry().GetTypeDescFromGUID(componentHeader.componentGUID));
			if (!typeDesc)
			{
				continue;
			}

			if (!ComponentRegistry::Helpers::HasComponentWithGUID(componentHeader.componentGUID, registry, entity.GetHandle()))
			{
				ComponentRegistry::Helpers::AddComponentWithGUID(componentHeader.componentGUID, registry, entity.GetHandle());
			}

			void* voidCompPtr = ComponentRegistry::Helpers::GetComponentWithGUID(componentHeader.componentGUID, registry, entity.GetHandle());
			uint8_t* componentData = reinterpret_cast<uint8_t*>(voidCompPtr);

			serializationData.componentData.Seek(componentHeader.componentDataOffset);

			DeserializeClass(serializationData.componentData, typeDesc, componentData);
		}
	}

	void DeserializeEntity(Entity entity, SerializationData& serializationData)
	{
		auto& registry = entity.GetSceneReference()->GetRegistry();

		for (const ComponentHeader& componentHeader : serializationData.componentHeaders)
		{
			const IComponentTypeDesc* typeDesc = reinterpret_cast<const IComponentTypeDesc*>(GetComponentRegistry().GetTypeDescFromGUID(componentHeader.componentGUID));
			if (!typeDesc)
			{
				continue;
			}

			void* voidCompPtr = ComponentRegistry::Helpers::GetComponentWithGUID(componentHeader.componentGUID, registry, entity.GetHandle());
			uint8_t* componentData = reinterpret_cast<uint8_t*>(voidCompPtr);

			serializationData.componentData.Seek(componentHeader.componentDataOffset);

			DeserializeClass(serializationData.componentData, typeDesc, componentData);
		}
	}

	void DeserializeEntityData(Archive& archive, SerializationData& outSerializationData)
	{
		VT_ENSURE(archive.IsLoading());

		archive << outSerializationData.entityId;
		archive << outSerializationData.ownerSceneAssetHandle;
		archive << outSerializationData.componentHeaders;
		archive << outSerializationData.componentData;
	}
}
