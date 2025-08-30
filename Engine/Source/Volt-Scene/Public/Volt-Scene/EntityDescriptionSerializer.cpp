#include "vspch.h"
#include "EntityDescriptionSerializer.h"

#include "Volt-Scene/EntityDescription.h"
#include "Volt-Scene/EntityDescCustomMetadata.h"
#include "Volt-Scene/Scene.h"
#include "Volt-Scene/Entity.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetSerializerRegistry.h>

#include <CoreUtilities/FileIO/YAMLMemoryStreamWriter.h>
#include <CoreUtilities/FileIO/YAMLMemoryStreamReader.h>
#include <CoreUtilities/Profiling/Profiling.h>

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
		RegisterSerializationFunction<int8_t>(s_typeSerializers);
		RegisterSerializationFunction<uint8_t>(s_typeSerializers);
		RegisterSerializationFunction<int16_t>(s_typeSerializers);
		RegisterSerializationFunction<uint16_t>(s_typeSerializers);
		RegisterSerializationFunction<int32_t>(s_typeSerializers);
		RegisterSerializationFunction<uint32_t>(s_typeSerializers);

		RegisterSerializationFunction<float>(s_typeSerializers);
		RegisterSerializationFunction<double>(s_typeSerializers);
		RegisterSerializationFunction<bool>(s_typeSerializers);

		RegisterSerializationFunction<glm::vec2>(s_typeSerializers);
		RegisterSerializationFunction<glm::vec3>(s_typeSerializers);
		RegisterSerializationFunction<glm::vec4>(s_typeSerializers);

		RegisterSerializationFunction<glm::uvec2>(s_typeSerializers);
		RegisterSerializationFunction<glm::uvec3>(s_typeSerializers);
		RegisterSerializationFunction<glm::uvec4>(s_typeSerializers);

		RegisterSerializationFunction<glm::ivec2>(s_typeSerializers);
		RegisterSerializationFunction<glm::ivec3>(s_typeSerializers);
		RegisterSerializationFunction<glm::ivec4>(s_typeSerializers);

		RegisterSerializationFunction<glm::quat>(s_typeSerializers);
		RegisterSerializationFunction<glm::mat4>(s_typeSerializers);
		RegisterSerializationFunction<VoltGUID>(s_typeSerializers);

		RegisterSerializationFunction<std::string>(s_typeSerializers);
		RegisterSerializationFunction<std::filesystem::path>(s_typeSerializers);

		RegisterSerializationFunction<Volt::EntityID>(s_typeSerializers);
		RegisterSerializationFunction<AssetHandle>(s_typeSerializers);

		RegisterDeserializationFunction<int8_t>(s_typeDeserializers);
		RegisterDeserializationFunction<uint8_t>(s_typeDeserializers);
		RegisterDeserializationFunction<int16_t>(s_typeDeserializers);
		RegisterDeserializationFunction<uint16_t>(s_typeDeserializers);
		RegisterDeserializationFunction<int32_t>(s_typeDeserializers);
		RegisterDeserializationFunction<uint32_t>(s_typeDeserializers);

		RegisterDeserializationFunction<float>(s_typeDeserializers);
		RegisterDeserializationFunction<double>(s_typeDeserializers);
		RegisterDeserializationFunction<bool>(s_typeDeserializers);

		RegisterDeserializationFunction<glm::vec2>(s_typeDeserializers);
		RegisterDeserializationFunction<glm::vec3>(s_typeDeserializers);
		RegisterDeserializationFunction<glm::vec4>(s_typeDeserializers);

		RegisterDeserializationFunction<glm::uvec2>(s_typeDeserializers);
		RegisterDeserializationFunction<glm::uvec3>(s_typeDeserializers);
		RegisterDeserializationFunction<glm::uvec4>(s_typeDeserializers);

		RegisterDeserializationFunction<glm::ivec2>(s_typeDeserializers);
		RegisterDeserializationFunction<glm::ivec3>(s_typeDeserializers);
		RegisterDeserializationFunction<glm::ivec4>(s_typeDeserializers);

		RegisterDeserializationFunction<glm::quat>(s_typeDeserializers);
		RegisterDeserializationFunction<glm::mat4>(s_typeDeserializers);
		RegisterDeserializationFunction<VoltGUID>(s_typeDeserializers);

		RegisterDeserializationFunction<std::string>(s_typeDeserializers);
		RegisterDeserializationFunction<std::filesystem::path>(s_typeDeserializers);

		RegisterDeserializationFunction<Volt::EntityID>(s_typeDeserializers);
		RegisterDeserializationFunction<AssetHandle>(s_typeDeserializers);

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

		//get the path of the directory this asset is in
		std::filesystem::path directoryPath = AssetManager::GetFilesystemPath(entityDesc->GetSceneHandle());
		if (!std::filesystem::is_directory(directoryPath))
		{
			directoryPath = directoryPath.parent_path();
		}
		directoryPath /= "Entities";

		if (!std::filesystem::exists(directoryPath))
		{
			std::filesystem::create_directories(directoryPath);
		}

		std::filesystem::path entityPath = directoryPath / (metadata.filePath.stem().string() + ".vtasset");


		//serialize entity data
		YAMLMemoryStreamWriter streamWriter{};
		Ref<Scene> scene = AssetManager::Get().GetAsset<Scene>(entityDesc->GetSceneHandle());
		SerializeEntity(entityDesc->m_entityID, scene, streamWriter);

		//write to file
		BinaryStreamWriter entityDescFileWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(metadata, asset->GetVersion(), entityDescFileWriter);

		Buffer buffer = streamWriter.WriteAndGetBuffer();
		entityDescFileWriter.Write(buffer);
		buffer.Release();

		entityDescFileWriter.WriteToDisk(entityPath, true, compressedDataOffset);
	}

	bool EntityDescSerializer::Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const
	{
		const EntityDescCustomMetadata& customMeta = metadata.GetCustomData<EntityDescCustomMetadata>();
		Ref<EntityDesc> entityDesc = std::reinterpret_pointer_cast<EntityDesc>(destinationAsset);
		entityDesc->m_sceneHandle = customMeta.sceneHandle;

		//if the scene is not loaded here, the entity cannot be loaded
		VT_ENSURE(AssetManager::Get().IsLoaded(customMeta.sceneHandle));
		Ref<Scene> scene = AssetManager::Get().GetAsset<Scene>(customMeta.sceneHandle);

		BinaryStreamReader streamReader{ metadata.filePath };
		Buffer buffer{};
		streamReader.Read(buffer);

		YAMLMemoryStreamReader yamlStreamReader{};
		yamlStreamReader.ConsumeBuffer(buffer);
		Entity resultEntity = DeserializeEntity(scene, yamlStreamReader);

		entityDesc->m_entityID = resultEntity.GetID();
		return true;
	}

	void EntityDescSerializer::SerializeEntity(Volt::EntityID id, const Ref<Scene>& scene, YAMLMemoryStreamWriter& streamWriter) const
	{
		streamWriter.BeginMap();
		streamWriter.BeginMapNamned("Entity");

		auto& registry = scene->GetRegistry();

		Entity entity = scene->GetEntityFromID(id);

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
					if (!ComponentRegistry::Helpers::HasComponentWithGUID(compGuid, scene->GetRegistry(), entity))
					{
						ComponentRegistry::Helpers::AddComponentWithGUID(compGuid, scene->GetRegistry(), entity);
					}

					void* voidCompPtr = ComponentRegistry::Helpers::GetComponentWithGUID(compGuid, scene->GetRegistry(), entity);
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
				if (s_typeSerializers.contains(member.typeIndex))
				{
					s_typeSerializers.at(member.typeIndex)(streamWriter, data, offset + member.offset);
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

		if (!isNonDefaultType && !s_typeSerializers.contains(typeIndex))
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
				if (s_typeSerializers.contains(typeIndex))
				{
					s_typeSerializers.at(typeIndex)(streamWriter, elementData, 0);
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
				if (s_typeDeserializers.contains(componentMember->typeIndex))
				{
					s_typeDeserializers.at(componentMember->typeIndex)(streamReader, data, offset + componentMember->offset);
				}
			}
		});

		compDesc->OnComponentDeserialized(dstEntity.GetScene()->GetEntityHelperFromEntityID(dstEntity.GetID()));
	}

	void EntityDescSerializer::DeserializeArray(uint8_t* data, const size_t offset, const IArrayTypeDesc* arrayDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const
	{
		void* arrayPtr = &data[offset];

		const bool isNonDefaultType = arrayDesc->GetElementTypeDesc() != nullptr;
		const auto& typeIndex = arrayDesc->GetElementTypeIndex();

		if (!isNonDefaultType && !s_typeSerializers.contains(typeIndex))
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
				if (s_typeDeserializers.contains(typeIndex))
				{
					s_typeDeserializers.at(typeIndex)(streamReader, tempBytePtr, 0);
				}
			}

			arrayDesc->PushBack(arrayPtr, tempDataStorage);
			arrayDesc->DestroyElement(tempDataStorage);
		});
	}
}
