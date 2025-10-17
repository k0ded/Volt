#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <AssetSystem/Asset.h>
#include <CoreUtilities/Containers/ArrayView.h>

namespace Volt
{
	struct Project;

	class Scene;
	class Prefab;
	class MeshAsset;
}

class ProjectConversionPanel : public EditorWindow
{
public:
	ProjectConversionPanel();

	void UpdateMainContent() override;

private:
	void TryConvertProject();
	bool TryLoadProject(Volt::Project& project);
	void TryConvertAssets(const Volt::Project& project, const ArrayView<Volt::AssetMetadata>& assetMetadata);

	Ref<Volt::Scene> TryConvertScene(const Volt::Project& project, const Volt::AssetMetadata& metadata, const Map<Volt::AssetHandle, Ref<Volt::Prefab>>& prefabs);
	Ref<Volt::MeshAsset> TryConvertMesh(const Volt::Project& project, const Volt::AssetMetadata& metadata);
	Ref<Volt::Prefab> TryConvertPrefab(const Volt::Project& project, const Volt::AssetMetadata& metadata);

	void PrintMissingMembers();

	void LoadAssetMetadataFromMetaFiles(const Volt::Project& project, Vector<Volt::AssetMetadata>& outMetadata);

	std::filesystem::path m_projectToConvertFilepath;
	std::filesystem::path m_targetDirectory;
};
