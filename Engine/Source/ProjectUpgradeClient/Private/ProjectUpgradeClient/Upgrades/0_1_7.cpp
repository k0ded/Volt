#include "Upgrades/0_1_7.h"

#include "UpgradesRegistry.h"

#include <Volt-Core/Project/Project.h>
#include <Volt-Core/Project/ProjectManager.h>

#include <AssetSystem/AssetMetadata.h>
#include <AssetSystem/Serialization/AssetSerializationCommon.h>
#include <AssetSystem/Serialization/AssetSerializer.h>
#include <AssetSystem/AssetSerializerRegistry.h>
#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetLocks.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/Archive/FileArchive.h>

namespace Volt
{
	REGISTER_UPGRADE(Version::Create(0, 1, 7), Upgrade_0_1_7);
	
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
				AssetRegistryIteratorFilter filter{};
				filter.includeMemoryAssets = false;
				filter.includeWithoutFilepath = false;

				g_assetManager->IterateAssetRegistryWithFilter(filter, [this](ReadOnlyAssetMetadata assetMetadata)
				{
					m_assetsToProcess.emplace_back(assetMetadata->handle);
					return true;
				});

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
		ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(assetHandle);

		if (!AssetSerializerRegistry::Get().HasSerializer(assetMetadata->type))
		{
			return;
		}

		AssetReference<Asset> asset = g_assetManager->CreateAssetTypeless("TempAsset", assetMetadata->type);
		AssetSerializerRegistry::Get().GetSerializer(assetMetadata->type).Deserialize(assetMetadata, asset);

		FileWriter fileWriter;
		if (!fileWriter.Open(GetTargetProject().rootDirectory / assetMetadata->filepath))
		{
			return;
		}

		asset->Serialize(fileWriter);
	}
}
