#include "ProjectUpgradeClient/Upgrades/0_1_7/Serializers/SceneSerializer.h"

#define private public
#include <Volt-Scene/Scene.h>
#undef private

#include <Volt-Scene/EntityDescription.h>
#include <Volt-Scene/WorldEngine/WorldCell.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

#include <Volt-Core/Project/ProjectManager.h>
#include <Volt-Core/Algorithms.h>

#include <EntitySystem/Entity.h>
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

	void SceneSerializer::Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
		const AssetReference<Scene> scene = asset.ConvertTo<Scene>();
		ScopedAssetReferenceLock sceneLock{ scene };

		std::filesystem::path directoryPath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);
		if (!std::filesystem::is_directory(directoryPath))
		{
			directoryPath = directoryPath.parent_path();
		}

		if (!std::filesystem::exists(directoryPath))
		{
			std::filesystem::create_directories(directoryPath);
		}

		std::filesystem::path scenePath = directoryPath / (metadata->filepath.stem().string() + ".vtasset");

		// Serialize scene file
		{
			YAMLMemoryStreamWriter streamWriter{};
			streamWriter.BeginMap();
			streamWriter.BeginMapNamned("Scene");
			streamWriter.SetKey("name", metadata->filepath.stem().string());

			streamWriter.BeginMapNamned("Settings");
			streamWriter.SetKey("useWorldEngine", scene->m_sceneSettings.useWorldEngine);
			streamWriter.EndMap();

			//todo: world engine
			/*if (scene->m_sceneSettings.useWorldEngine)
			{
				SerializeWorldEngine(scene, streamWriter);
			}*/

			streamWriter.EndMap();
			streamWriter.EndMap();

			BinaryStreamWriter sceneFileWriter{};
			const size_t compressedDataOffset = AssetSerializer::WriteMetadata(*metadata, asset->GetVersion(), sceneFileWriter);

			auto buffer = streamWriter.WriteAndGetBuffer();
			sceneFileWriter.Write(buffer);
			buffer.Release();

			sceneFileWriter.WriteToDisk(scenePath, true, compressedDataOffset);
		}
	}

	bool SceneSerializer::Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const
	{
		AssetReference<Scene> scene = destinationAsset.ConvertTo<Scene>();
		ScopedAssetReferenceLock sceneLock{ scene };

		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);

		if (!std::filesystem::exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata->filepath);
			scene->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader streamReader{ filePath };
		if (!streamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file {0}!", metadata->filepath);
			scene->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);
		VT_ASSERT_MSG(serializedMetadata.version == scene->GetVersion(), "Incompatible version!");

		// Scene File
		{
			Buffer buffer{};
			streamReader.Read(buffer);

			YAMLMemoryStreamReader yamlStreamReader{};
			if (!yamlStreamReader.ConsumeBuffer(buffer))
			{
				scene->SetFlag(AssetFlag::Invalid, true);
				return false;
			}

			yamlStreamReader.EnterScope("Scene");
			scene->m_name = yamlStreamReader.ReadAtKey("name", std::string("New Scene"));

			yamlStreamReader.EnterScope("Settings");
			{
				scene->m_sceneSettings.useWorldEngine = yamlStreamReader.ReadAtKey("useWorldEngine", true);
			}
			yamlStreamReader.ExitScope();

			//todo: world engine
			/*if (scene->m_sceneSettings.useWorldEngine)
			{
				DeserializeWorldEngine(scene, yamlStreamReader);
			}*/

			yamlStreamReader.ExitScope();
		}

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

	//todo: wordl engine
	//void SceneSerializer::SerializeWorldEngine(const Ref<Scene>& scene, YAMLMemoryStreamWriter& streamWriter) const
	//{
	//	auto& worldEngine = scene->m_worldEngine;

	//	streamWriter.BeginMapNamned("WorldEngine");
	//	streamWriter.SetKey("cellSize", worldEngine.GetSettings().cellSize);
	//	streamWriter.SetKey("worldSize", worldEngine.GetSettings().worldSize);

	//	streamWriter.EndMap();
	//}

	//void SceneSerializer::DeserializeWorldEngine(const Ref<Scene>& scene, YAMLMemoryStreamReader& streamReader) const
	//{
	//	auto& worldEngine = scene->m_worldEngine;

	//	streamReader.EnterScope("WorldEngine");
	//	worldEngine.GetSettingsMutable().cellSize = streamReader.ReadAtKey("cellSize", 25'600);
	//	worldEngine.GetSettingsMutable().worldSize = streamReader.ReadAtKey("worldSize", glm::uvec2{ 128'000 });
	//	worldEngine.GenerateCells();
	//	streamReader.ExitScope();
	//}
}
