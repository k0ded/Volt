#include "vspch.h"

#include "Volt-Scene/EntityDescSerialization.h"
#include "Volt-Scene/EntityDescCustomMetadata.h"

#include <EntitySystem/ComponentRegistry.h>
#include <EntitySystem/Scripting/CoreComponents.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetTypes.h>

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
			memberHeaders.emplace_back(member.identifier, member.size, componentMemberDataWriter.GetHeadLocation());

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
		Vector<MemberHeader, InlineAllocator<MAX_COMPONENT_COUNT>> memberHeaders;
		MemoryReader componentMemberDataReader;

		archive << memberHeaders;
		archive << componentMemberDataReader;

		for (const auto& member : memberHeaders)
		{
			const ComponentMember* componentMember = componentDesc->FindMemberByIdentifier(member.identifier);
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
				// #TODO_Ivar: Disabled for now as some types can have different sizes depending on configuration.
				//if (componentMember->size != member.size)
				//{
				//	continue;
				//}

				componentMember->serializeFunction(componentMemberDataReader, componentMemberDataPtr);
			}
		}
	}

	void SerializeEntity(Archive& archive, Entity entity, AssetHandle ownerSceneAssetHandle)
	{
		// This should only be used for SAVING out entities.
		VT_ENSURE(!archive.IsLoading());

		EntityID entityId = entity.GetID();

		SerializationData entityData;
		entityData.entityId = entityId;
		entityData.ownerSceneAssetHandle = ownerSceneAssetHandle;
		GatherComponentData(entity, entityData.components);

		SerializeEntityDescData(archive, entityData);
	}

	void DeserializeEntity(Archive& archive, Entity entity)
	{
		VT_ENSURE(archive.IsLoading());

		SerializationData serializationData;
		SerializeEntityDescData(archive, serializationData);

		ApplyEntityDescData(entity, serializationData);
	}


	void ApplyEntityDescData(Entity entity, SerializationData& serializationData)
	{
		ApplyComponentData(entity, serializationData.components);
		entity.GetComponent<IDComponent>().id = serializationData.entityId;
	}

	VTS_API void SerializeEntityDescData(Archive& archive, SerializationData& entityData)
	{
		archive.UseVersion(EntityDescArchiveVersion::guid);

		archive << entityData.entityId;
		archive << entityData.ownerSceneAssetHandle;
		const int32_t entityDescVersion = archive.GetVersion(EntityDescArchiveVersion::guid);

		archive << entityData.components.headers;

		if (archive.IsLoading())
		{
			if (entityDescVersion >= EntityDescArchiveVersion::AllowModifyingPartsOfSavedata)
			{
				archive << entityData.components.data;
			}
			else
			{
				MemoryReader reader;
				archive << reader;
				//the archive originally had no version, so we need to remove the 8 bytes that correspond to the versions count (size_t) in the beginning
				const size_t byteCount = reader.GetSize() - 8;
				entityData.components.data.resize_uninitialized(byteCount);
				memcpy_s(entityData.components.data.data(), entityData.components.data.size(), reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(reader.GetData()) + 8), byteCount);
			}
		}
		else
		{
			archive << entityData.components.data;
		}
	}

	VTS_API void GatherComponentData(Entity entity, ComponentData& outComponentData)
	{
		outComponentData.headers.clear();
		outComponentData.data.clear();

		VersionlessMemoryWriterExternal componentDataWriter(outComponentData.data);

		entt::registry& registry = entity.GetSceneReference()->GetRegistry();

		for (auto&& curr : registry.storage())
		{
			auto& storage = curr.second;

			if (!storage.contains(entity.GetHandle()))
			{
				// Entity does not have this component, skip
				continue;
			}

			const IComponentTypeDesc* componentDesc = static_cast<const IComponentTypeDesc*>(ComponentRegistry::Get().GetTypeDescFromName(storage.type().name()));
			if (!componentDesc)
			{
				// Component isn't registered, skip
				continue;
			}

			const size_t startSize = componentDataWriter.GetSize();
			const size_t startHeadLoc = componentDataWriter.GetHeadLocation();

			const uint8_t* componentPtr = reinterpret_cast<const uint8_t*>(storage.get(entity.GetHandle()));
			SerializeClass(componentDataWriter, componentDesc, componentPtr);

			// Write the header.
			const size_t componentDataSize = componentDataWriter.GetSize() - startSize;
			VT_ENSURE(componentDataSize > 0);
			outComponentData.headers.emplace_back(componentDesc->GetGUID(), startHeadLoc, componentDataSize);
		}

		componentDataWriter.Close();
		outComponentData.data.resize_uninitialized(componentDataWriter.GetSize());
		memcpy_s(outComponentData.data.data(), outComponentData.data.size(), componentDataWriter.GetData(), componentDataWriter.GetSize());
	}

	void UpdateSingleComponentInData(Entity entity, VoltGUID componentToUpdate, ComponentData& outComponentData)
	{
		const bool entityHasComponent = entity.HasComponent(componentToUpdate);
		bool dataHasComponent = false;

		int32_t dataComponentHeaderIndex = -1;
		for (int32_t i = 0; i < outComponentData.headers.size(); i++)
		{
			const ComponentHeader& header = outComponentData.headers[i];
			if (header.componentGUID == componentToUpdate)
			{
				dataHasComponent = true;
				dataComponentHeaderIndex = i;
				break;
			}
		}

		const bool wasAdded = entityHasComponent && !dataHasComponent;
		const bool wasRemoved = !entityHasComponent && dataHasComponent;
		VT_ENSURE_MSG(!(wasAdded && wasRemoved), "Component cannot have been added and removed at the same time!");

		const IComponentTypeDesc* componentDesc = static_cast<const IComponentTypeDesc*>(ComponentRegistry::Get().GetTypeDescFromGUID(componentToUpdate));
		entt::registry& registry = entity.GetSceneReference()->GetRegistry();
		const uint8_t* componentPtr = reinterpret_cast<const uint8_t*>(ComponentRegistry::Helpers::GetComponentWithGUID(componentToUpdate, registry, entity.GetHandle()));
		if (wasAdded)
		{
			//serialize the component to the end of the data
			VersionlessMemoryWriterExternal componentDataWriter(outComponentData.data);
			const size_t startSize = componentDataWriter.GetSize();
			SerializeClass(componentDataWriter, componentDesc, componentPtr);
			componentDataWriter.Close();

			//add the new header at the end of the component data headers
			const size_t componentDataSize = componentDataWriter.GetSize() - startSize;
			VT_ENSURE(componentDataSize > 0);
			outComponentData.headers.emplace_back(componentToUpdate, startSize, componentDataSize);

			return;
		}

		VT_ENSURE(dataComponentHeaderIndex != -1);
		ComponentHeader& header = outComponentData.headers[dataComponentHeaderIndex];
		if (wasRemoved)
		{
			//set size of component range to 0 before erasing it
			UpdateHeaderByteRangeSize(outComponentData, header, 0);

			// has to be at the end of scope to preserve the reference to header while we use it
			outComponentData.headers.erase_unsorted(outComponentData.headers.begin() + dataComponentHeaderIndex);
			return;
		}

		//at this point we know the component already exists in the data and on the entity
		Vector<uint8_t> bytes;
		VersionlessMemoryWriterExternal componentDataWriter(bytes);
		SerializeClass(componentDataWriter, componentDesc, componentPtr);
		componentDataWriter.Close();

		UpdateHeaderByteRangeSize(outComponentData, header, componentDataWriter.GetSize());
		memcpy_s(outComponentData.data.data() + header.componentDataOffset, header.componentDataSize, componentDataWriter.GetData(), componentDataWriter.GetSize());
	}

	void UpdateHeaderByteRangeSize(ComponentData& componentData, ComponentHeader& header, size_t newSize)
	{
		VT_ENSURE((header.componentDataOffset + header.componentDataSize) <= componentData.data.size());

		const int64_t sizeDiff = newSize - header.componentDataSize;

		//if the size is already correct, do nothing
		if (sizeDiff == 0)
		{
			return;
		}

		//if header byte range is between other ranges, move the bytes coming after the range
		size_t numAfterEndOfRange = componentData.data.size() - (header.componentDataOffset + header.componentDataSize);
		if (numAfterEndOfRange > 0)
		{
			//if the size is increasing, need to allocate space for when the data is moved
			if (sizeDiff > 0)
			{
				componentData.data.resize_uninitialized(componentData.data.size() + sizeDiff);
				numAfterEndOfRange += sizeDiff;
			}

			//move the data after the specified header
			uint8_t* const startPtr = componentData.data.data() + header.componentDataOffset;
			uint8_t* const endPtr = startPtr + header.componentDataSize;

			uint8_t* const newEndPtr = startPtr + newSize;
			memmove_s(newEndPtr, numAfterEndOfRange + header.componentDataSize - sizeDiff, endPtr, numAfterEndOfRange);

			//if the size decreased, remove the now excess from the end
			if (sizeDiff < 0)
			{
				componentData.data.resize_uninitialized(componentData.data.size() + sizeDiff);
			}
		}

		header.componentDataSize = newSize;

		//also update the other headers
		for (int i = 0; i < componentData.headers.size(); i++)
		{
			ComponentHeader& updatingHeader = componentData.headers[i];
			if (updatingHeader.componentDataOffset > header.componentDataOffset)
			{
				updatingHeader.componentDataOffset += sizeDiff;
			}
		}
	}

	VTS_API void ApplyComponentData(Entity entity, ComponentData& componentData)
	{
		auto& registry = entity.GetSceneReference()->GetRegistry();

		VersionlessMemoryReaderExternal reader(componentData.data);
		for (const ComponentHeader& componentHeader : componentData.headers)
		{
			const IComponentTypeDesc* typeDesc = reinterpret_cast<const IComponentTypeDesc*>(ComponentRegistry::Get().GetTypeDescFromGUID(componentHeader.componentGUID));
			if (!typeDesc)
			{
				continue;
			}

			if (!ComponentRegistry::Helpers::HasComponentWithGUID(componentHeader.componentGUID, registry, entity.GetHandle()))
			{
				ComponentRegistry::Helpers::AddComponentWithGUID(componentHeader.componentGUID, registry, entity.GetHandle());
			}

			void* voidCompPtr = ComponentRegistry::Helpers::GetComponentWithGUID(componentHeader.componentGUID, registry, entity.GetHandle());
			uint8_t* byteCompPtr = reinterpret_cast<uint8_t*>(voidCompPtr);

			reader.Seek(componentHeader.componentDataOffset);

			DeserializeClass(reader, typeDesc, byteCompPtr);
		}
	}

	std::filesystem::path GetSavePathForEntity(const Volt::AssetHandle& handle)
	{
		ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(handle);
		VT_ENSURE(assetMetadata->type == AssetTypes::EntityDesc);

		const EntityDescCustomMetadata& entityMetadata = assetMetadata->GetCustomData<EntityDescCustomMetadata>();

		ReadOnlyAssetMetadata sceneAssetMetadata = g_assetManager->GetReadOnlyAssetMetadata(entityMetadata.sceneHandle);

		VT_ENSURE(sceneAssetMetadata->HasFilepath());

		const std::filesystem::path& owningScenePath = sceneAssetMetadata->filepath;
		const std::string owningSceneName = owningScenePath.stem().string();

		const std::filesystem::path relativePath = owningScenePath.parent_path() / (owningSceneName + "_Entities") / (std::to_string(entityMetadata.entityID) + ".vtasset");
		return g_assetManager->GetAssetFilesystemPath(relativePath);
	}
}
