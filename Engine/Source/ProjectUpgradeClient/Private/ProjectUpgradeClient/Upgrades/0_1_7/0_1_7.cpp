#include "Upgrades/0_1_7/0_1_7.h"
#include "Upgrades/0_1_7/Serializers/EntityDescriptionSerializer.h"
#include "Upgrades/0_1_7/AssetSerializerRegistry.h"
#include "Common/YAMLMemoryStreamReader.h"

#include "UpgradesRegistry.h"

#include <Volt-Scene/AssetTypes.h>
#include <Volt-Scene/EntityDescription.h>
#include <Volt-Scene/EntityDescCustomMetadata.h>
#include <Volt-Scene/Scene.h>

#include <Volt-Core/Project/Project.h>
#include <Volt-Core/Project/ProjectManager.h>

#include <Volt-FileSystem/FileArchive.h>
#include <Volt-FileSystem/Filesystem.h>
#include <Volt-FileSystem/Iterators/RecursiveDirectoryIterator.h>

#include <AssetSystem/AssetMetadata.h>
#include <AssetSystem/AssetFactory.h>

#include <EntitySystem/Entity.h>

#include <JobSystem/TaskGraph.h>

namespace Volt
{
	REGISTER_UPGRADE(Version::Create(0, 1, 7), Upgrade_0_1_7);
	
	static void DeserializeAssetMetadata(AssetMetadata_0_1_7& outMetadata, const Filesystem::Path& assetFilepath)
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
		g_assetManager = CreateUnique<AssetManager>(Filesystem::GetWorkingDirectory(), inProject.rootDirectory, inProject.assetsDirectoryName);
	}

	Upgrade_0_1_7::~Upgrade_0_1_7()
	{
		m_assetsToKeepLoaded.clear();
		g_assetManager.Reset();
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

	String Upgrade_0_1_7::GetCurrentActionText()
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
		AssetMetadata_0_1_7& assetMetadata = m_assetHandleToMetadata.at(assetHandle);

		if (!AssetSerializerRegistry::Get().HasSerializer(assetMetadata.type))
		{
			return;
		}

		AssetReference<Asset> asset = g_assetManager->CreateAssetTypeless("TempAsset", assetMetadata.type);

		// If it's a entity desc asset the scene must be loaded when serialized.
		// Make sure the scene is loaded.
		AssetReference<Scene> sceneAsset;
		if (asset->GetType() == AssetTypes::EntityDesc)
		{
			AssetReference<EntityDesc> entityDescAsset = asset.ConvertTo<EntityDesc>();

			const EntityDescCustomMetadata& customMeta = assetMetadata.GetCustomData<EntityDescCustomMetadata>();

			bool sceneIsLoaded = false;

			for (AssetReference<Asset> loadedAsset : m_assetsToKeepLoaded)
			{
				if (loadedAsset->GetAssetHandle() == customMeta.sceneHandle)
				{
					sceneIsLoaded = true;
					break;
				}
			}

			if (!sceneIsLoaded && m_assetHandleToMetadata.contains(customMeta.sceneHandle))
			{
				sceneAsset = g_assetManager->CreateAssetTypeless("TempAsset", AssetTypes::Scene).ConvertTo<Scene>();
				AssetMetadata_0_1_7& sceneMetadata = m_assetHandleToMetadata.at(customMeta.sceneHandle);

				AssetSerializerRegistry::Get().GetSerializer(sceneMetadata.type).Deserialize(&sceneMetadata, sceneAsset);
				m_assetsToKeepLoaded.emplace_back(sceneAsset);
			}

			if (!sceneAsset.IsValid())
			{
				return;
			}

			//entityDescAsset->AssignOwnerScene(sceneAsset);
		}
		else if (asset->GetType() == AssetTypes::Scene)
		{
			m_assetsToKeepLoaded.emplace_back(asset);
		}

		AssetSerializerRegistry::Get().GetSerializer(assetMetadata.type).Deserialize(&assetMetadata, asset);

		if (asset->GetType() == AssetTypes::EntityDesc)
		{
			AssetReference<EntityDesc> entityDescAsset = asset.ConvertTo<EntityDesc>();

			YAMLMemoryStreamReader yamlStreamReader;
			//yamlStreamReader.ReadBuffer(entityDescAsset->GetEntitySpawnData());
			EntityDescSerializer::Get().DeserializeEntity(sceneAsset, yamlStreamReader);
		}

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

		asset->Serialize(fileWriter, ReadOnlyAssetMetadata(AssetMetadataInit::Null));
		fileWriter.Close();
	}

	void Upgrade_0_1_7::LoadAssetMetadatas()
	{
		constexpr StringView AssetExtension = ".vtasset";
		constexpr uint32_t NumEngineFilepaths = 2;

		const Array<Filesystem::Path, NumEngineFilepaths> engineFilepathsToScan =
		{
			Filesystem::GetWorkingDirectory() / "Engine",
			GetTargetProject().rootDirectory / "Editor",
		};

		const Filesystem::Path projectFilepathToScan = GetTargetProject().rootDirectory / GetTargetProject().assetsDirectoryName;

		TaskGraph scanGraph{ ExecutionPriority::Immediate };

		Array<Vector<Filesystem::Path>, NumEngineFilepaths> engineIntermediateFilepaths;

		for (uint32_t index = 0; const Filesystem::Path& filepathToScan : engineFilepathsToScan)
		{
			// If the directory does not exist, we skip.
			if (!Filesystem::Exists(filepathToScan))
			{
				continue;
			}

			scanGraph.AddTask("Scan Engine Assets", [&engineIntermediateFilepaths, &filepathToScan, index]()
			{
				for (const auto& pathIt : Filesystem::RecursiveDirectoryIterator(filepathToScan))
				{
					if (pathIt.path.Extension() == AssetExtension)
					{
						engineIntermediateFilepaths[index].emplace_back(pathIt.path);
					}
				}
			});

			index++;
		}

		Vector<Filesystem::Path> assets;

		// Make sure the project assets directory exists.
		if (Filesystem::Exists(projectFilepathToScan))
		{
			scanGraph.AddTask("Scan Project Assets", [&assets, &projectFilepathToScan]()
			{
				for (const auto& pathIt : Filesystem::RecursiveDirectoryIterator(projectFilepathToScan))
				{
					if (pathIt.path.Extension() == AssetExtension)
					{
						assets.emplace_back(pathIt.path);
					}
				}
			});
		}

		scanGraph.ExecuteAndWait();

		for (const Vector<Filesystem::Path>& intermediate : engineIntermediateFilepaths)
		{
			assets.append(intermediate);
		}

		for (const Filesystem::Path& assetFilepath : assets)
		{
			AssetMetadata_0_1_7 metadata;
			DeserializeAssetMetadata(metadata, assetFilepath);

			if (metadata.handle != Asset::Null())
			{
				m_assetsToProcess.emplace_back(metadata.handle);
				m_assetHandleToMetadata[metadata.handle] = metadata;
			}
		}
	}
}
