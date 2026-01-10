#pragma once
#include "UpgradeInterface.h"

#include <AssetSystem/AssetManager.h>
#include <CoreUtilities/Containers/ArrayView.h>

namespace Volt
{
	struct Project;

	class Scene;
	class Prefab;
	class MeshAsset;

	class LegacyProjectUpgrade : public Upgrade
	{
	public:
		LegacyProjectUpgrade(const Project& inProject);
		~LegacyProjectUpgrade() override = default;

		void TryConvertProject(const std::filesystem::path& projectFilepath, const std::filesystem::path& targetDirectory);

		bool ProcessUpgrade() override;
		size_t GetNumTotalActions() override;
		size_t GetNumActionsCompleted() override;
		std::string GetCurrentActionText() override;

	private:
		bool TryLoadProject(Volt::Project& project);
		void TryConvertAssets(const Volt::Project& project, const ArrayView<Volt::AssetMetadata>& assetMetadata);

		Vector<AssetReference<Volt::Asset>> TryConvertScene(const Volt::Project& project, const Volt::AssetMetadata& metadata, const Map<Volt::AssetHandle, AssetReference<Volt::Prefab>>& prefabs);
		AssetReference<Volt::MeshAsset> TryConvertMesh(const Volt::Project& project, const Volt::AssetMetadata& metadata);
		AssetReference<Volt::Prefab> TryConvertPrefab(const Volt::Project& project, const Volt::AssetMetadata& metadata);

		void PrintMissingMembers();

		void LoadAssetMetadataFromMetaFiles(const Volt::Project& project, Vector<Volt::AssetMetadata>& outMetadata);

		std::filesystem::path m_projectToConvertFilepath;
		std::filesystem::path m_targetDirectory;

		Scope<Volt::AssetManager> m_assetManager;
	};
}
