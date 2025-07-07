#include "vspch.h"

#include "Volt-Scene/SceneSerializer.h"
#include "Volt-Scene/Scene.h"
#include "Volt-Scene/Entity.h"
#include "Volt-Scene/WorldEngine/WorldCell.h"

#include <AssetSystem/AssetManager.h>

#include <Volt-Core/Project/ProjectManager.h>
#include <Volt-Core/Algorithms.h>

#include <EntitySystem/ComponentRegistry.h>

#include <CoreUtilities/FileIO/YAMLMemoryStreamWriter.h>
#include <CoreUtilities/FileIO/YAMLMemoryStreamReader.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/FileSystem.h>

namespace Volt
{
	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::Scene, SceneSerializer);

	SceneSerializer::SceneSerializer()
	{
		s_instance = this;
	}

	SceneSerializer::~SceneSerializer()
	{
		s_instance = nullptr;
	}

	void SceneSerializer::Serialize(const AssetMetadata& metadata, StackVector<uint8_t, ASSET_METADATA_SIZE>& customData, const Ref<Asset>& asset) const
	{
		const Ref<Scene> scene = std::reinterpret_pointer_cast<Scene>(asset);

		std::filesystem::path directoryPath = AssetManager::GetFilesystemPath(metadata.filePath);
		if (!std::filesystem::is_directory(directoryPath))
		{
			directoryPath = directoryPath.parent_path();
		}

		if (!std::filesystem::exists(directoryPath))
		{
			std::filesystem::create_directories(directoryPath);
		}

		std::filesystem::path scenePath = directoryPath / (metadata.filePath.stem().string() + ".vtasset");

		// Serialize scene file
		{
			YAMLMemoryStreamWriter streamWriter{};
			streamWriter.BeginMap();
			streamWriter.BeginMapNamned("Scene");
			streamWriter.SetKey("name", metadata.filePath.stem().string());

			streamWriter.BeginMapNamned("Settings");
			streamWriter.SetKey("useWorldEngine", scene->m_sceneSettings.useWorldEngine);
			streamWriter.EndMap();

			if (scene->m_sceneSettings.useWorldEngine)
			{
				SerializeWorldEngine(scene, streamWriter);
			}

			streamWriter.EndMap();
			streamWriter.EndMap();

			BinaryStreamWriter sceneFileWriter{};
			const size_t compressedDataOffset = AssetSerializer::WriteMetadata(metadata, asset->GetVersion(), sceneFileWriter);

			auto buffer = streamWriter.WriteAndGetBuffer();
			sceneFileWriter.Write(buffer);
			buffer.Release();

			sceneFileWriter.WriteToDisk(scenePath, true, compressedDataOffset);
		}
	}

	bool SceneSerializer::Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const
	{
		Ref<Scene> scene = reinterpret_pointer_cast<Scene>(destinationAsset);

		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);

		if (!std::filesystem::exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata.filePath);
			destinationAsset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader streamReader{ filePath };
		if (!streamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file {0}!", metadata.filePath);
			destinationAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);
		VT_ASSERT_MSG(serializedMetadata.version == destinationAsset->GetVersion(), "Incompatible version!");

		// Scene File
		{
			Buffer buffer{};
			streamReader.Read(buffer);

			YAMLMemoryStreamReader yamlStreamReader{};
			if (!yamlStreamReader.ConsumeBuffer(buffer))
			{
				destinationAsset->SetFlag(AssetFlag::Invalid, true);
				return false;
			}

			yamlStreamReader.EnterScope("Scene");
			scene->m_name = yamlStreamReader.ReadAtKey("name", std::string("New Scene"));

			yamlStreamReader.EnterScope("Settings");
			{
				scene->m_sceneSettings.useWorldEngine = yamlStreamReader.ReadAtKey("useWorldEngine", true);
			}
			yamlStreamReader.ExitScope();

			if (scene->m_sceneSettings.useWorldEngine)
			{
				DeserializeWorldEngine(scene, yamlStreamReader);
			}

			yamlStreamReader.ExitScope();
		}

		//const std::filesystem::path& scenePath = filePath;
		//std::filesystem::path directoryPath = scenePath.parent_path();

		//LoadCellEntities(metadata, scene, directoryPath);

		scene->SortScene();
		return true;
	}

	/*void SceneSerializer::LoadWorldCell(const Ref<Scene>& scene, const WorldCell& worldCell) const
	{
		VT_PROFILE_FUNCTION();

		if (worldCell.isLoaded)
		{
			VT_LOG(Warning, "[SceneImporter]: World Cell is already loaded!");
			return;
		}

		if (worldCell.cellEntities.empty())
		{
			VT_LOG(Warning, "[SceneImporter]: Unable to load World Cell which contains zero entities!");
			return;
		}

		const auto& metadata = AssetManager::GetMetadataFromHandle(scene->handle);
		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);
		const std::filesystem::path sceneDirectory = filePath.parent_path();

		std::filesystem::path layersFolderPath = sceneDirectory / "Entities";
		if (!std::filesystem::exists(layersFolderPath))
		{
			return;
		}

		Vector<std::filesystem::path> entityPaths;

		for (const auto& it : std::filesystem::directory_iterator(layersFolderPath))
		{
			if (!it.is_directory() && it.path().extension().string() == ENTITY_FILE_EXTENSION)
			{
				entityPaths.emplace_back(it.path());
			}
		}

		for (int32_t i = static_cast<int32_t>(entityPaths.size()) - 1; i >= 0; i--)
		{
			const auto& path = entityPaths.at(i);

			const std::string stem = path.stem().string();
			uint32_t entityId = std::stoul(stem);
			auto it = std::ranges::find(worldCell.cellEntities, EntityID(entityId));

			if (it == worldCell.cellEntities.end())
			{
				entityPaths.erase(entityPaths.begin() + i);
			}
		}

		if (entityPaths.empty())
		{
			return;
		}

		const uint32_t threadCount = Algo::GetThreadCountFromIterationCount(static_cast<uint32_t>(entityPaths.size()));

		Vector<Ref<Scene>> dummyScenes{};
		dummyScenes.resize(threadCount);

		Algo::ForEachParallelLocking([&dummyScenes, entityPaths, metadata, this](uint32_t threadIdx, uint32_t i)
		{
			if (!dummyScenes[threadIdx])
			{
				dummyScenes[threadIdx] = CreateRef<Scene>();;
			}

			const auto& path = entityPaths.at(i);

			BinaryStreamReader streamReader{ path };
			if (!streamReader.IsStreamValid())
			{
				return;
			}

			uint32_t magicVal = 0;
			streamReader.Read(magicVal);

			if (magicVal != ENTITY_MAGIC_VAL)
			{
				VT_LOG(Error, "[SceneSerializer]: File is not a valid entity!");
				return;
			}

			Buffer buffer{};
			streamReader.Read(buffer);

			YAMLMemoryStreamReader yamlStreamReader{};
			if (!yamlStreamReader.ConsumeBuffer(buffer))
			{
				return;
			}

			DeserializeEntity(dummyScenes[threadIdx], metadata, yamlStreamReader);
		},
		static_cast<uint32_t>(entityPaths.size()));

		for (const auto& dummyScene : dummyScenes)
		{
			if (!dummyScene)
			{
				continue;
			}

			for (const auto& entity : dummyScene->GetAllEntities())
			{
				Entity realEntity = scene->CreateEntityWithID(entity.GetID());
				Entity::Copy(entity, realEntity, Volt::EntityCopyFlags::None);

				scene->InvalidateEntityTransform(realEntity.GetID());
			}
		}
	}*/

	void SceneSerializer::SerializeWorldEngine(const Ref<Scene>& scene, YAMLMemoryStreamWriter& streamWriter) const
	{
		auto& worldEngine = scene->m_worldEngine;

		streamWriter.BeginMapNamned("WorldEngine");
		streamWriter.SetKey("cellSize", worldEngine.GetSettings().cellSize);
		streamWriter.SetKey("worldSize", worldEngine.GetSettings().worldSize);

		streamWriter.EndMap();
	}

	void SceneSerializer::DeserializeWorldEngine(const Ref<Scene>& scene, YAMLMemoryStreamReader& streamReader) const
	{
		auto& worldEngine = scene->m_worldEngine;

		streamReader.EnterScope("WorldEngine");
		worldEngine.GetSettingsMutable().cellSize = streamReader.ReadAtKey("cellSize", 25'600);
		worldEngine.GetSettingsMutable().worldSize = streamReader.ReadAtKey("worldSize", glm::uvec2{ 128'000 });
		worldEngine.GenerateCells();
		streamReader.ExitScope();
	}
}
