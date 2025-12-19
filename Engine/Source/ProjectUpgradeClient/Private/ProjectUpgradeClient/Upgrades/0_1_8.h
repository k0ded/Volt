#pragma once

#include "UpgradeInterface.h"

#include <AssetSystem/AssetManager.h>

namespace Volt
{
	class Upgrade_0_1_8 : public Upgrade
	{
	public:
		Upgrade_0_1_8(const Project& inProject);
		~Upgrade_0_1_8() override;

		bool ProcessUpgrade() override;

		// Inherited via Upgrade
		size_t GetNumTotalActions() override;
		size_t GetNumActionsCompleted() override;

		std::string GetCurrentActionText() override;

	private:
		inline static constexpr size_t ASSET_CUSTOM_METADATA_SIZE = 256;
		typedef Vector<uint8_t, InlineAllocator<ASSET_CUSTOM_METADATA_SIZE>> CustomAssetMetadataVector;

		void DeserializeAssetMetadata(AssetMetadata& outMetadata, const std::filesystem::path& assetFilepath);

		enum class UpgradeStage
		{
			Collecting,
			Converting,
			Done
		};

		void LoadAssetMetadatas();
		void ProcessAsset(AssetHandle assetHandle);

		UpgradeStage m_currentStage;

		size_t m_numTotalActions;
		size_t m_numActionsCompleted;

		Vector<AssetHandle> m_assetsToProcess;
		Vector<AssetReference<Asset>> m_sceneAssets;
		Map<AssetHandle, AssetMetadata> m_assetHandleToMetadata;
		Map<AssetHandle, CustomAssetMetadataVector> m_assetHandleToCustomMetadata;
	};
}
