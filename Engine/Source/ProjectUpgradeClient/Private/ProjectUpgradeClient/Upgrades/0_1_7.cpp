#include "Upgrades/0_1_7.h"

#include "UpgradesRegistry.h"

#include <Volt-Scene/AssetTypes.h>
#include <Volt-Scene/EntityDescription.h>
#include <Volt-Scene/Scene.h>

#include <Volt-Core/Project/Project.h>
#include <Volt-Core/Project/ProjectManager.h>

#include <AssetSystem/AssetMetadata.h>
#include <AssetSystem/Serialization/AssetSerializationCommon.h>
#include <AssetSystem/Serialization/AssetSerializer.h>
#include <AssetSystem/AssetSerializerRegistry.h>
#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetLocks.h>

#include <JobSystem/TaskGraph.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/Archive/FileArchive.h>

namespace Volt
{
	REGISTER_UPGRADE(Version::Create(0, 1, 7), Upgrade_0_1_7);
	
	static void DeserializeAssetMetadata(AssetMetadata& outMetadata, const std::filesystem::path& assetFilepath)
	{
		constexpr size_t assetHeaderSize = SerializedAssetMetadata::HeaderSize;

		outMetadata.handle = Asset::Null();

		BinaryStreamReader streamReader{ assetFilepath, assetHeaderSize };
		if (!streamReader.IsStreamValid())
		{
			VT_LOGC(Error, LogAssetSystem, "Failed to open asset file: {0}!", assetFilepath);
			return;
		}

		uint32_t value = 0;
		bool couldReadValue = streamReader.TryRead(value);
		if (!couldReadValue || value != SerializedAssetMetadata::AssetMagic)
		{
			VT_LOGC(Error, LogAssetSystem, "File {} is not a valid Volt asset!", assetFilepath);
			return;
		}

		streamReader.ResetHead();

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);

		outMetadata.handle = serializedMetadata.handle;
		outMetadata.filepath = g_assetManager->GetRelativeAssetFilepath(assetFilepath);
		outMetadata.type = serializedMetadata.type;
		outMetadata.customData = serializedMetadata.customData;
	}

	Upgrade_0_1_7::Upgrade_0_1_7(const Project& inProject)
		: Upgrade(inProject)
	{
		m_currentStage = UpgradeStage::Collecting;
		m_numActionsCompleted = 0;
		m_numTotalActions = 0;

		// Serializers rely on the global asset manager existing.
		g_assetManager = CreateScope<AssetManager>(std::filesystem::current_path(), inProject.rootDirectory, inProject.assetsDirectoryName);
	}

	Upgrade_0_1_7::~Upgrade_0_1_7()
	{
		g_assetManager.reset();
	}

	bool Upgrade_0_1_7::ProcessUpgrade()
	{
		switch (m_currentStage)
		{
			case UpgradeStage::Collecting:
			{
				LoadAssetMetadatas();

				m_currentStage = UpgradeStage::Converting;
				m_numTotalActions = m_assetsToProcess.size();
				break;
			}

			case UpgradeStage::Converting:
			{
				constexpr size_t NumAssetsPerFrame = 25;

				for (size_t i = 0; i < NumAssetsPerFrame; ++i)
				{
					if (m_assetsToProcess.empty())
					{
						break;
					}

					AssetHandle assetHandle = m_assetsToProcess.back();
					m_assetsToProcess.pop_back();

					ProcessFile(assetHandle);
				}

				if (m_assetsToProcess.empty())
				{
					m_currentStage = UpgradeStage::Done;
				}

				break;
			}
		}

		return m_currentStage == UpgradeStage::Done;
	}

	size_t Upgrade_0_1_7::GetNumTotalActions()
	{
		return m_numTotalActions;
	}

	size_t Upgrade_0_1_7::GetNumActionsCompleted()
	{
		return m_numActionsCompleted;
	}

	std::string Upgrade_0_1_7::GetCurrentActionText()
	{
		switch (m_currentStage)
		{
			case UpgradeStage::Converting: return "Converting files...";
		}
		return "Error";
	}

	void Upgrade_0_1_7::ProcessFile(AssetHandle asset)
	{
		ProcessAsset(asset);
		m_numActionsCompleted++;
	}

	void Upgrade_0_1_7::ProcessAsset(AssetHandle assetHandle)
	{
		AssetMetadata& assetMetadata = m_assetHandleToMetadata.at(assetHandle);

		if (!AssetSerializerRegistry::Get().HasSerializer(assetMetadata.type))
		{
			return;
		}

		AssetReference<Asset> asset = g_assetManager->CreateAssetTypeless("TempAsset", assetMetadata.type);
		ScopedAssetReferenceLock assetLock{ asset };

		// If it's a entity desc asset the scene must be loaded when serialized.
		// Make sure the scene is loaded.
		AssetReference<Asset> sceneAsset;

		if (asset->GetType() == AssetTypes::EntityDesc)
		{
			AssetReference<EntityDesc> entityDescAsset = asset.ConvertTo<EntityDesc>();
			ScopedAssetReferenceLock entityDescLock{ entityDescAsset };

			bool sceneIsLoaded = false;

			for (AssetReference<Asset> loadedAsset : m_assetsToKeepLoaded)
			{
				ScopedAssetReferenceLock loadedAssetLock{ loadedAsset };

				if (loadedAsset->GetAssetHandle() == entityDescAsset->GetSceneHandle())
				{
					sceneIsLoaded = true;
					break;
				}
			}

			if (!sceneIsLoaded)
			{
				sceneAsset = g_assetManager->CreateAssetTypeless("TempAsset", AssetTypes::Scene);
				AssetMetadata& sceneMetadata = m_assetHandleToMetadata.at(entityDescAsset->GetSceneHandle());

				AssetSerializerRegistry::Get().GetSerializer(assetMetadata.type).Deserialize(ReadOnlyAssetMetadata(&sceneMetadata), sceneAsset);
			}

			if (!sceneAsset.IsValid())
			{
				return;
			}
		}

		AssetSerializerRegistry::Get().GetSerializer(assetMetadata.type).Deserialize(ReadOnlyAssetMetadata(&assetMetadata), asset);

		FileWriter fileWriter;
		if (!fileWriter.Open(g_assetManager->GetAssetFilesystemPath(assetMetadata.filepath)))
		{
			return;
		}

		uint32_t assetMagic = AssetManager::AssetFileMagic;
		uint32_t assetVersion = asset->GetVersion();

		fileWriter << assetMagic;
		fileWriter << assetVersion;
		fileWriter << assetMetadata;

		asset->Serialize(fileWriter);
		fileWriter.Close();
	}

	void Upgrade_0_1_7::LoadAssetMetadatas()
	{
		constexpr std::string_view AssetExtension = ".vtasset";
		constexpr uint32_t NumEngineFilepaths = 2;

		const Array<std::filesystem::path, NumEngineFilepaths> engineFilepathsToScan =
		{
			std::filesystem::current_path() / "Engine",
			GetTargetProject().rootDirectory / "Editor",
		};

		const std::filesystem::path projectFilepathToScan = GetTargetProject().rootDirectory / GetTargetProject().assetsDirectoryName;

		TaskGraph scanGraph{ ExecutionPriority::Immediate };

		Array<Vector<std::filesystem::path>, NumEngineFilepaths> engineIntermediateFilepaths;

		for (uint32_t index = 0; const std::filesystem::path& filepathToScan : engineFilepathsToScan)
		{
			// If the directory does not exist, we skip.
			if (!FileSystem::Exists(filepathToScan))
			{
				continue;
			}

			scanGraph.AddTask("Scan Engine Assets", [&engineIntermediateFilepaths, &filepathToScan, index]()
			{
				for (const auto& pathIt : std::filesystem::recursive_directory_iterator(filepathToScan))
				{
					if (pathIt.path().extension() == AssetExtension)
					{
						engineIntermediateFilepaths[index].emplace_back(pathIt.path());
					}
				}
			});

			index++;
		}

		Vector<std::filesystem::path> assets;

		// Make sure the project assets directory exists.
		if (FileSystem::Exists(projectFilepathToScan))
		{
			scanGraph.AddTask("Scan Project Assets", [&assets, &projectFilepathToScan]()
			{
				for (const auto& pathIt : std::filesystem::recursive_directory_iterator(projectFilepathToScan))
				{
					if (pathIt.path().extension() == AssetExtension)
					{
						assets.emplace_back(pathIt.path());
					}
				}
			});
		}

		scanGraph.ExecuteAndWait();

		for (const Vector<std::filesystem::path>& intermediate : engineIntermediateFilepaths)
		{
			assets.append(intermediate);
		}

		for (const std::filesystem::path& assetFilepath : assets)
		{
			AssetMetadata metadata;
			DeserializeAssetMetadata(metadata, assetFilepath);

			m_assetsToProcess.emplace_back(metadata.handle);
			m_assetHandleToMetadata[metadata.handle] = metadata;
		}
	}
}
