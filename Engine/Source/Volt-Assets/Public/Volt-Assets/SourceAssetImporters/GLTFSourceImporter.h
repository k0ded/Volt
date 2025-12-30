#pragma once

#include "Volt-Assets/SourceAssetImporters/ImportConfigs.h"

#include <AssetSystem/SourceAssetImporter.h>
#include <AssetSystem/SourceAssetImporterRegistry.h>

VT_DECLARE_LOG_CATEGORY(LogGLTFSourceImporter, LogVerbosity::Trace);

namespace fastgltf
{
	class Asset;
	struct Node;
}

struct TQS;

namespace Volt
{
	class Mesh;
	class MaterialAsset;
	class MeshInitializer;

	class GLTFSourceImporter final : public SourceAssetImporter
	{
	protected:
		Vector<AssetReference<Asset>> ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const override;
		SourceAssetFileInformation GetSourceFileInformation(const std::filesystem::path& filepath) const override;

	private:
		Vector<AssetReference<Asset>> ProcessTextures(fastgltf::Asset& gltfAsset, const std::filesystem::path& srcDirectory, const MeshSourceImportConfig& config) const;
		Vector<AssetReference<Asset>> ImportAsStaticMesh(fastgltf::Asset& gltfAsset, const MeshSourceImportConfig& config, const SourceAssetUserImportData& userData, const Vector<AssetReference<Asset>>& importedTextures) const;
		
		void CreateVoltMeshFromGLTFMesh(size_t gltfNodeIndex, const fastgltf::Asset& gltfAsset, const Map<size_t, TQS>& nodeGlobalTransform, MeshInitializer& meshInitializer, const MeshSourceImportConfig& importConfig, const Vector<AssetReference<MaterialAsset>>& materials) const;
	};
}
