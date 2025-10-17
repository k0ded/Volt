#include "vspch.h"
#include "EntityDescriptionSerializer.h"

#include "Volt-Scene/EntityDescription.h"
#include "Volt-Scene/EntityDescCustomMetadata.h"
#include "Volt-Scene/Scene.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetSerializerRegistry.h>

#include <CoreUtilities/FileIO/YAMLMemoryStreamWriter.h>
#include <CoreUtilities/FileIO/YAMLMemoryStreamReader.h>
#include <CoreUtilities/Profiling/Profiling.h>

#include <EntitySystem/Entity.h>
#include <EntitySystem/ComponentRegistry.h>

#include <Volt-Platforms/Windows/WindowsPlatformThread.h>


namespace Volt
{
	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::EntityDesc, EntityDescSerializer);

	template<typename T>
	void RegisterSerializationFunction(std::unordered_map<TypeTraits::TypeIndex, std::function<void(YAMLMemoryStreamWriter&, const uint8_t*, const size_t)>>& outTypes)
	{
		VT_PROFILE_FUNCTION();

		outTypes[TypeTraits::TypeIndex::FromType<T>()] = [](YAMLMemoryStreamWriter& streamWriter, const uint8_t* data, const size_t offset)
		{
			const T& var = *reinterpret_cast<const T*>(&data[offset]);
			streamWriter.SetKey("data", var);
		};
	}

	template<typename T>
	void RegisterDeserializationFunction(std::unordered_map<TypeTraits::TypeIndex, std::function<void(YAMLMemoryStreamReader&, uint8_t*, const size_t)>>& outTypes)
	{
		VT_PROFILE_FUNCTION();

		outTypes[TypeTraits::TypeIndex::FromType<T>()] = [](YAMLMemoryStreamReader& streamReader, uint8_t* data, const size_t offset)
		{
			*reinterpret_cast<T*>(&data[offset]) = streamReader.ReadAtKey("data", T());
		};
	}

	EntityDescSerializer::EntityDescSerializer()
	{
		RegisterSerializationFunction<int8_t>(m_typeSerializers);
		RegisterSerializationFunction<uint8_t>(m_typeSerializers);
		RegisterSerializationFunction<int16_t>(m_typeSerializers);
		RegisterSerializationFunction<uint16_t>(m_typeSerializers);
		RegisterSerializationFunction<int32_t>(m_typeSerializers);
		RegisterSerializationFunction<uint32_t>(m_typeSerializers);

		RegisterSerializationFunction<float>(m_typeSerializers);
		RegisterSerializationFunction<double>(m_typeSerializers);
		RegisterSerializationFunction<bool>(m_typeSerializers);

		RegisterSerializationFunction<glm::vec2>(m_typeSerializers);
		RegisterSerializationFunction<glm::vec3>(m_typeSerializers);
		RegisterSerializationFunction<glm::vec4>(m_typeSerializers);

		RegisterSerializationFunction<glm::uvec2>(m_typeSerializers);
		RegisterSerializationFunction<glm::uvec3>(m_typeSerializers);
		RegisterSerializationFunction<glm::uvec4>(m_typeSerializers);

		RegisterSerializationFunction<glm::ivec2>(m_typeSerializers);
		RegisterSerializationFunction<glm::ivec3>(m_typeSerializers);
		RegisterSerializationFunction<glm::ivec4>(m_typeSerializers);

		RegisterSerializationFunction<glm::quat>(m_typeSerializers);
		RegisterSerializationFunction<glm::mat4>(m_typeSerializers);
		RegisterSerializationFunction<VoltGUID>(m_typeSerializers);

		RegisterSerializationFunction<std::string>(m_typeSerializers);
		RegisterSerializationFunction<std::filesystem::path>(m_typeSerializers);

		RegisterSerializationFunction<Volt::EntityID>(m_typeSerializers);
		RegisterSerializationFunction<AssetHandle>(m_typeSerializers);

		RegisterDeserializationFunction<int8_t>(m_typeDeserializers);
		RegisterDeserializationFunction<uint8_t>(m_typeDeserializers);
		RegisterDeserializationFunction<int16_t>(m_typeDeserializers);
		RegisterDeserializationFunction<uint16_t>(m_typeDeserializers);
		RegisterDeserializationFunction<int32_t>(m_typeDeserializers);
		RegisterDeserializationFunction<uint32_t>(m_typeDeserializers);

		RegisterDeserializationFunction<float>(m_typeDeserializers);
		RegisterDeserializationFunction<double>(m_typeDeserializers);
		RegisterDeserializationFunction<bool>(m_typeDeserializers);

		RegisterDeserializationFunction<glm::vec2>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::vec3>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::vec4>(m_typeDeserializers);

		RegisterDeserializationFunction<glm::uvec2>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::uvec3>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::uvec4>(m_typeDeserializers);

		RegisterDeserializationFunction<glm::ivec2>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::ivec3>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::ivec4>(m_typeDeserializers);

		RegisterDeserializationFunction<glm::quat>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::mat4>(m_typeDeserializers);
		RegisterDeserializationFunction<VoltGUID>(m_typeDeserializers);

		RegisterDeserializationFunction<std::string>(m_typeDeserializers);
		RegisterDeserializationFunction<std::filesystem::path>(m_typeDeserializers);

		RegisterDeserializationFunction<Volt::EntityID>(m_typeDeserializers);
		RegisterDeserializationFunction<AssetHandle>(m_typeDeserializers);

		s_instance = this;
	}

	EntityDescSerializer::~EntityDescSerializer()
	{}

	void EntityDescSerializer::Serialize(const AssetMetadata& metadata, CustomAssetMetadataVector& customData, const Ref<Asset>& asset) const
	{
		const Ref<EntityDesc> entityDesc = std::reinterpret_pointer_cast<EntityDesc>(asset);

		//if the scene is not loaded here, the entity is not supposed to be loaded, and cannot be saved
		VT_ENSURE(AssetManager::Get().IsLoaded(entityDesc->GetSceneHandle()));
		//if the scene is a memory asset it doesnt have a path yet, and will thus fail the save of this entity
		VT_ENSURE(!AssetManager::Get().IsMemoryAsset(entityDesc->GetSceneHandle()));

		const std::filesystem::path directoryPath = metadata.filePath.parent_path();
		if (!std::filesystem::exists(directoryPath))
		{
			std::filesystem::create_directories(directoryPath);
		}


		//serialize entity data
		YAMLMemoryStreamWriter streamWriter{};
		Ref<Scene> scene = AssetManager::Get().GetAsset<Scene>(entityDesc->GetSceneHandle());
		SerializeEntity(scene->GetEntityFromID(entityDesc->m_entityID), streamWriter);

		//write to file
		BinaryStreamWriter entityDescFileWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(metadata, asset->GetVersion(), entityDescFileWriter);

		Buffer buffer = streamWriter.WriteAndGetBuffer();
		entityDescFileWriter.Write(buffer);
		buffer.Release();

		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);
		entityDescFileWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool EntityDescSerializer::Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const
	{
		const EntityDescCustomMetadata& customMeta = metadata.GetCustomData<EntityDescCustomMetadata>();
		Ref<EntityDesc> entityDesc = std::reinterpret_pointer_cast<EntityDesc>(destinationAsset);
		entityDesc->m_sceneHandle = customMeta.sceneHandle;
		entityDesc->m_entityID = customMeta.entityID;




		BinaryStreamReader streamReader{ AssetManager::GetFilesystemPath(metadata.filePath) };
		AssetSerializer::ReadMetadata(streamReader);

		entityDesc->m_entitySpawnData.Clear();
		streamReader.Read(entityDesc->m_entitySpawnData);		
		return true;
	}

	void EntityDescSerializer::SerializeEntity(Entity entity, YAMLMemoryStreamWriter& streamWriter) const
	{
		streamWriter.BeginMap();
		streamWriter.BeginMapNamned("Entity");

		entt::registry& registry = entity.GetSceneReference()->GetRegistry();

		streamWriter.SetKey("id", entity.GetID());
		streamWriter.BeginSequence("components");
		{
			for (auto&& curr : registry.storage())
			{
				auto& storage = curr.second;

				if (!storage.contains(entity.GetHandle()))
				{
					continue;
				}

				const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(GetComponentRegistry().GetTypeDescFromName(storage.type().name()));
				if (!componentDesc)
				{
					continue;
				}

				const uint8_t* componentPtr = reinterpret_cast<const uint8_t*>(storage.get(entity.GetHandle()));
				SerializeClass(componentPtr, 0, componentDesc, streamWriter, false);
			}
		}
		streamWriter.EndSequence();

		// #TODO_Scene
		//if (registry.any_of<VertexPaintedComponent>(id))
		//{
		//	std::filesystem::path vpPath = (ProjectManager::GetRootDirectory() / metadata.filePath.parent_path() / "Layers" / ("ent_" + std::to_string((uint32_t)id) + ".entVp"));
		//	auto& vpComp = registry.get<VertexPaintedComponent>(id);
		//
		//	// #TODO_Ivar: This is kind of questionable after TGA
		//	if (std::filesystem::exists(vpPath))
		//	{
		//		using std::filesystem::perms;
		//		std::filesystem::permissions(vpPath, perms::_All_write);
		//	}
		//
		//	BinaryStreamWriter vpStreamWriter;
		//
		//	vpStreamWriter.Write(vpComp.vertexColors.data(), sizeof(uint32_t) * vpComp.vertexColors.size());
		//	vpStreamWriter.Write(vpComp.meshHandle);
		//
		//	vpStreamWriter.WriteToDisk(vpPath, false, 0);
		//}

		streamWriter.EndMap();
		streamWriter.EndMap();
	}



	Entity EntityDescSerializer::DeserializeEntity(const Ref<Scene>& scene, YAMLMemoryStreamReader& streamReader) const
	{
		streamReader.EnterScope("Entity");

		EntityID entityId = streamReader.ReadAtKey("id", Entity::NullID());

		if (entityId == Entity::NullID())
		{
			return Entity::Null();
		}

		Entity entity = CreateEntityFromUUIDThreadSafe(entityId, scene);

		streamReader.ForEach("components", [&]()
		{
			VT_PROFILE_SCOPE("Component");

			VoltGUID compGuid = streamReader.ReadAtKey("guid", VoltGUID::Null());
			if (compGuid == VoltGUID::Null())
			{
				return;
			}

			const ICommonTypeDesc* typeDesc = GetComponentRegistry().GetTypeDescFromGUID(compGuid);
			if (!typeDesc)
			{
				return;
			}

			switch (typeDesc->GetValueType())
			{
				case ValueType::Component:
				{
					auto& registry = scene->GetEntityScene().GetRegistry();

					if (!ComponentRegistry::Helpers::HasComponentWithGUID(compGuid, registry, entity.GetHandle()))
					{
						ComponentRegistry::Helpers::AddComponentWithGUID(compGuid, registry, entity.GetHandle());
					}

					void* voidCompPtr = ComponentRegistry::Helpers::GetComponentWithGUID(compGuid, registry, entity.GetHandle());
					uint8_t* componentData = reinterpret_cast<uint8_t*>(voidCompPtr);

					const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);
					DeserializeClass(componentData, 0, componentDesc, entity, streamReader);
					break;
				}
			}

		});

		// #TODO_Scene
		//if (scene->GetRegistry().any_of<VertexPaintedComponent>(entity))
		//{
		//	std::filesystem::path vpPath = metadata.filePath.parent_path();
		//	vpPath = ProjectManager::GetRootDirectory() / vpPath / "Layers" / ("ent_" + std::to_string((uint32_t)entityId) + ".entVp");

		//	if (std::filesystem::exists(vpPath))
		//	{
		//		auto& vpComp = scene->GetRegistry().get<VertexPaintedComponent>(entity);

		//		std::ifstream vpFile(vpPath, std::ios::in | std::ios::binary);
		//		if (!vpFile.is_open())
		//		{
		//			VT_LOG(Error, "Could not open entVp file!");
		//		}

		//		Vector<uint8_t> totalData;
		//		const size_t srcSize = vpFile.seekg(0, std::ios::end).tellg();
		//		totalData.resize_uninitialized(srcSize);
		//		vpFile.seekg(0, std::ios::beg);
		//		vpFile.read(reinterpret_cast<char*>(totalData.data()), totalData.size());
		//		vpFile.close();

		//		memcpy_s(&vpComp.meshHandle, sizeof(vpComp.meshHandle), totalData.data() + totalData.size() - sizeof(vpComp.meshHandle), sizeof(vpComp.meshHandle));
		//		totalData.resize_uninitialized(totalData.size() - sizeof(vpComp.meshHandle));

		//		vpComp.vertexColors.reserve(totalData.size() / sizeof(uint32_t));
		//		for (size_t offset = 0; offset < totalData.size(); offset += sizeof(uint32_t))
		//		{
		//			uint32_t vpColor;
		//			memcpy_s(&vpColor, sizeof(uint32_t), totalData.data() + offset, sizeof(uint32_t));
		//			vpComp.vertexColors.push_back(vpColor);
		//		}
		//	}
		//}

		streamReader.ExitScope();

		return entity;
	}

	void EntityDescSerializer::DeserializeEntityInPlace(Volt::Entity entity, YAMLMemoryStreamReader& streamReader) const
	{
		streamReader.EnterScope("Entity");

		EntityID entityId = streamReader.ReadAtKey("id", Entity::NullID());
		VT_ENSURE_MSG(entityId == entity.GetID(), std::format("tried to deserialize entity with ID '{0}' onto an entity with ID '{1}'", std::to_string(entityId), std::to_string(entity.GetID())));

		streamReader.ForEach("components", [&]()
		{
			VoltGUID compGuid = streamReader.ReadAtKey("guid", VoltGUID::Null());
			if (compGuid == VoltGUID::Null())
			{
				return;
			}

			const ICommonTypeDesc* typeDesc = GetComponentRegistry().GetTypeDescFromGUID(compGuid);
			if (!typeDesc)
			{
				return;
			}
			switch (typeDesc->GetValueType())
			{
				case ValueType::Component:
				{
					entt::registry& registry = entity.GetSceneReference()->GetRegistry();
					const bool hasComponent = ComponentRegistry::Helpers::HasComponentWithGUID(compGuid, registry, entity.GetHandle());
					VT_ENSURE_MSG(hasComponent, "All components for entity have to be created beforehand in order to use DeserializeEntityInPlace");

					void* voidCompPtr = ComponentRegistry::Helpers::GetComponentWithGUID(compGuid, registry, entity.GetHandle());
					uint8_t* componentData = reinterpret_cast<uint8_t*>(voidCompPtr);

					const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);
					DeserializeClass(componentData, 0, componentDesc, entity, streamReader);
					break;
				}
			}
		});
	}

	Vector<VoltGUID> EntityDescSerializer::FindComponentTypes(YAMLMemoryStreamReader& streamReader)
	{
		Vector<VoltGUID> result;

		streamReader.EnterScope("Entity");

		streamReader.ForEach("components", [&]()
		{
			VT_PROFILE_SCOPE("Component");

			VoltGUID compGuid = streamReader.ReadAtKey("guid", VoltGUID::Null());
			if (compGuid == VoltGUID::Null())
			{
				return;
			}

			const ICommonTypeDesc* typeDesc = GetComponentRegistry().GetTypeDescFromGUID(compGuid);
			if (!typeDesc)
			{
				return;
			}

			result.push_back(compGuid);
		});

		streamReader.ExitScope();

		return result;
	}

	std::filesystem::path EntityDescSerializer::GetSavePathForEntity_ThreadSafe(const Volt::AssetHandle& handle)
	{
		const Volt::AssetMetadata metadata = Volt::AssetManager::GetMetadataFromHandle(handle);
		VT_ENSURE(metadata.type == AssetTypes::EntityDesc);
		const EntityDescCustomMetadata& entityMetadata = metadata.GetCustomData<EntityDescCustomMetadata>();

		const Volt::AssetHandle& sceneHandle = entityMetadata.sceneHandle;
		VT_ENSURE(Volt::AssetManager::HasFilePath(sceneHandle));

		const std::filesystem::path owningScenePath = AssetManager::GetFilePathFromAssetHandle(sceneHandle);
		const std::string owningSceneName = owningScenePath.stem().string();

		const std::filesystem::path relativePath = owningScenePath.parent_path() / (owningSceneName + "_Entities") / (std::to_string(entityMetadata.entityID) + ".vtasset");
		return AssetManager::GetFilesystemPath(relativePath);
	}

	Entity EntityDescSerializer::CreateEntityFromUUIDThreadSafe(EntityID entityId, const Ref<Scene>& scene) const
	{
		static std::mutex createEntityMutex;
		std::scoped_lock lock{ createEntityMutex };

		return scene->CreateEntityWithID(entityId);
	}

	void EntityDescSerializer::SerializeClass(const uint8_t* data, const size_t offset, const IComponentTypeDesc* compDesc, YAMLMemoryStreamWriter& streamWriter, bool isSubComponent) const
	{
		if (!isSubComponent)
		{
			streamWriter.BeginMap();
		}

		streamWriter.SetKey("guid", compDesc->GetGUID());
		streamWriter.BeginSequence("members");

		for (const auto& member : compDesc->GetMembers())
		{
			if ((member.flags & ComponentMemberFlag::NoSerialize) != ComponentMemberFlag::None)
			{
				continue;
			}

			streamWriter.BeginMap();
			streamWriter.SetKey("name", member.name);

			if (member.typeDesc != nullptr)
			{
				switch (member.typeDesc->GetValueType())
				{
					case ValueType::Component:
						streamWriter.SetKey("data", "component");
						SerializeClass(data, offset + member.offset, reinterpret_cast<const IComponentTypeDesc*>(member.typeDesc), streamWriter, true);
						break;

					case ValueType::Enum:
						streamWriter.SetKey("data", "enum");
						streamWriter.SetKey("enumValue", *reinterpret_cast<const int32_t*>(&data[offset + member.offset]));
						break;

					case ValueType::Array:
					{
						streamWriter.SetKey("data", "array");
						SerializeArray(data, offset + member.offset, reinterpret_cast<const IArrayTypeDesc*>(member.typeDesc), streamWriter);
						break;
					}
				}
			}
			else
			{
				if (m_typeSerializers.contains(member.typeIndex))
				{
					m_typeSerializers.at(member.typeIndex)(streamWriter, data, offset + member.offset);
				}
			}

			streamWriter.EndMap();
		}

		streamWriter.EndSequence();

		if (!isSubComponent)
		{
			streamWriter.EndMap();
		}
	}

	void EntityDescSerializer::SerializeArray(const uint8_t* data, const size_t offset, const IArrayTypeDesc* arrayDesc, YAMLMemoryStreamWriter& streamWriter) const
	{
		const void* arrayPtr = &data[offset];

		const bool isNonDefaultType = arrayDesc->GetElementTypeDesc() != nullptr;
		const auto& typeIndex = arrayDesc->GetElementTypeIndex();

		if (!isNonDefaultType && !m_typeSerializers.contains(typeIndex))
		{
			return;
		}

		streamWriter.BeginSequence("values");
		for (size_t i = 0; i < arrayDesc->Size(arrayPtr); i++)
		{
			const uint8_t* elementData = reinterpret_cast<const uint8_t*>(arrayDesc->At(arrayPtr, i));

			streamWriter.BeginMap();
			if (isNonDefaultType)
			{
				switch (arrayDesc->GetElementTypeDesc()->GetValueType())
				{
					case ValueType::Component:
						streamWriter.SetKey("value", "component");
						SerializeClass(elementData, 0, reinterpret_cast<const IComponentTypeDesc*>(arrayDesc->GetElementTypeDesc()), streamWriter, true);
						break;

					case ValueType::Enum:
						streamWriter.SetKey("value", *reinterpret_cast<const int32_t*>(elementData));
						break;

					case ValueType::Array:
						streamWriter.SetKey("value", "array");
						SerializeArray(elementData, 0, reinterpret_cast<const IArrayTypeDesc*>(arrayDesc->GetElementTypeDesc()), streamWriter);
						break;
				}
			}
			else
			{
				if (m_typeSerializers.contains(typeIndex))
				{
					m_typeSerializers.at(typeIndex)(streamWriter, elementData, 0);
				}
			}
			streamWriter.EndMap();
		}
		streamWriter.EndSequence();
	}

	void EntityDescSerializer::DeserializeClass(uint8_t* data, const size_t offset, const IComponentTypeDesc* compDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const
	{
		streamReader.ForEach("members", [&]()
		{
			const std::string memberName = streamReader.ReadAtKey("name", std::string(""));
			if (memberName.empty())
			{
				return;
			}

			const ComponentMember* componentMember = const_cast<IComponentTypeDesc*>(compDesc)->FindMemberByName(memberName);
			if (!componentMember)
			{
				return;
			}

			if (componentMember->typeDesc != nullptr)
			{
				switch (componentMember->typeDesc->GetValueType())
				{
					case ValueType::Component:
					{
						const IComponentTypeDesc* memberCompType = reinterpret_cast<const IComponentTypeDesc*>(componentMember->typeDesc);
						DeserializeClass(data, offset + componentMember->offset, memberCompType, dstEntity, streamReader);
						break;
					}

					case ValueType::Enum:
					{
						*reinterpret_cast<int32_t*>(&data[offset + componentMember->offset]) = streamReader.ReadAtKey("enumValue", int32_t(0));
						break;
					}

					case ValueType::Array:
					{
						const IArrayTypeDesc* arrayTypeDesc = reinterpret_cast<const IArrayTypeDesc*>(componentMember->typeDesc);
						DeserializeArray(data, offset + componentMember->offset, arrayTypeDesc, dstEntity, streamReader);
						break;
					}
				}
			}
			else
			{
				if (m_typeDeserializers.contains(componentMember->typeIndex))
				{
					m_typeDeserializers.at(componentMember->typeIndex)(streamReader, data, offset + componentMember->offset);
				}
			}
		});
	}

	void EntityDescSerializer::DeserializeArray(uint8_t* data, const size_t offset, const IArrayTypeDesc* arrayDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const
	{
		void* arrayPtr = &data[offset];

		const bool isNonDefaultType = arrayDesc->GetElementTypeDesc() != nullptr;
		const auto& typeIndex = arrayDesc->GetElementTypeIndex();

		if (!isNonDefaultType && !m_typeSerializers.contains(typeIndex))
		{
			return;
		}

		streamReader.ForEach("values", [&]()
		{
			void* tempDataStorage = nullptr;
			arrayDesc->DefaultConstructElement(tempDataStorage);

			uint8_t* tempBytePtr = reinterpret_cast<uint8_t*>(tempDataStorage);

			if (isNonDefaultType)
			{
				switch (arrayDesc->GetElementTypeDesc()->GetValueType())
				{
					case ValueType::Component:
					{
						const IComponentTypeDesc* compType = reinterpret_cast<const IComponentTypeDesc*>(arrayDesc->GetElementTypeDesc());
						DeserializeClass(tempBytePtr, 0, compType, dstEntity, streamReader);
						break;
					}

					case ValueType::Enum:
						*reinterpret_cast<int32_t*>(&tempDataStorage) = streamReader.ReadAtKey("enumValue", int32_t(0));
						break;

					case ValueType::Array:
					{
						const IArrayTypeDesc* arrayTypeDesc = reinterpret_cast<const IArrayTypeDesc*>(arrayDesc->GetElementTypeDesc());
						DeserializeArray(tempBytePtr, 0, arrayTypeDesc, dstEntity, streamReader);
						break;
					}
				}
			}
			else
			{
				if (m_typeDeserializers.contains(typeIndex))
				{
					m_typeDeserializers.at(typeIndex)(streamReader, tempBytePtr, 0);
				}
			}

			arrayDesc->PushBack(arrayPtr, tempDataStorage);
			arrayDesc->DestroyElement(tempDataStorage);
		});
	}
}
