#include "Upgrades/0_1_8.h"
#include "UpgradesRegistry.h"

#include <Volt-Scene/EntityDescription.h>
#include <Volt-Scene/EntityDescCustomMetadata.h>

#include <Volt-Core/Project/Project.h>
#include <JobSystem/TaskGraph.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/Archive/FileArchive.h>

namespace Volt
{
	Upgrade_0_1_8::Upgrade_0_1_8(const Project& inProject)
		: Upgrade(inProject)
	{
		m_currentStage = UpgradeStage::Collecting;
	
		// Serializers rely on the global asset manager existing.
		g_assetManager = CreateScope<AssetManager>(std::filesystem::current_path(), inProject.rootDirectory, inProject.assetsDirectoryName);
	}
	
	Upgrade_0_1_8::~Upgrade_0_1_8()
	{
		m_sceneAssets.clear();
		g_assetManager.reset();
	}

	void Upgrade_0_1_8::DeserializeAssetMetadata(AssetMetadata& outMetadata, const std::filesystem::path& assetFilepath)
	{
		FileReader fileReader;
		if (!fileReader.Open(assetFilepath))
		{
			VT_LOGC(Error, LogAssetSystem, "Failed to open asset file: '{}'\nError: {}", assetFilepath, fileReader.GetError());
			return;
		}

		// Deserialize the asset metadata header
		uint32_t assetMagic;
		uint32_t assetVersion;

		fileReader << assetMagic;

		if (assetMagic != AssetManager::AssetFileMagic)
		{
			return;
		}

		fileReader << assetVersion;

		// Deserialize the metadata itself
		fileReader << outMetadata.handle;

		VoltGUID assetTypeGUID;
		fileReader << assetTypeGUID;

		outMetadata.type = GetAssetTypeRegistry().GetTypeFromGUID(assetTypeGUID);

		if (fileReader.GetVersion(AssetMetadataArchiveVersion::guid) < AssetMetadataArchiveVersion::NewCustomMetadataStorage)
		{
			fileReader << m_assetHandleToCustomMetadata[outMetadata.handle];
		}
		else
		{
			fileReader << outMetadata.customData;
		}
	}

	bool Upgrade_0_1_8::ProcessUpgrade()
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

					ProcessAsset(assetHandle);
					m_numActionsCompleted++;
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
	
	size_t Upgrade_0_1_8::GetNumTotalActions()
	{
		return m_numTotalActions;
	}
	
	size_t Upgrade_0_1_8::GetNumActionsCompleted()
	{
		return m_numActionsCompleted;
	}
	
	std::string Upgrade_0_1_8::GetCurrentActionText()
	{
		switch (m_currentStage)
		{
			case UpgradeStage::Converting: return "Converting files...";
		}
		return "Error";
	}

	void Upgrade_0_1_8::LoadAssetMetadatas()
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

			if (metadata.handle != Asset::Null())
			{
				m_assetsToProcess.emplace_back(metadata.handle);
				m_assetHandleToMetadata[metadata.handle] = metadata;
			}

			if (metadata.type == AssetTypes::Scene)
			{
				AssetReference<Asset> sceneAsset;
				if (g_assetManager->TryGetTypelessAssetImmediately(metadata.handle, sceneAsset))
				{
					m_sceneAssets.emplace_back(sceneAsset);
				}
			}
		}
	}

	void Upgrade_0_1_8::ProcessAsset(AssetHandle assetHandle)
	{
		AssetReference<Asset> asset;
		if (!g_assetManager->TryGetTypelessAssetImmediately(assetHandle, asset))
		{
			return;
		}

		if (asset)
		{
			ScopedAssetReferenceLock lock{ asset };

			if (asset->GetType() == AssetTypes::EntityDesc)
			{
				if (m_assetHandleToCustomMetadata.contains(assetHandle))
				{
					const EntityDescCustomMetadata& oldCustomMetadata = *reinterpret_cast<const EntityDescCustomMetadata*>(m_assetHandleToCustomMetadata.at(assetHandle).data());

					WriteableAssetMetadata assetMetadata = g_assetManager->GetWriteableAssetMetadata(assetHandle);
					assetMetadata->customData.InitializeWithType<EntityDescCustomMetadata>();
					assetMetadata->customData.GetMutableCustomMetadata<EntityDescCustomMetadata>() = oldCustomMetadata;
				}
			}

			g_assetManager->SaveAsset(asset);
		}
	}
}
