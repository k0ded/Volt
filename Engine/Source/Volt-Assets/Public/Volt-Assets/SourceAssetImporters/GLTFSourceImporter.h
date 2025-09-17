#pragma once

#include "Volt-Assets/SourceAssetImporters/ImportConfigs.h"

#include <AssetSystem/SourceAssetImporter.h>
#include <AssetSystem/SourceAssetImporterRegistry.h>

#include <LogModule/LogCategory.h>

VT_DECLARE_LOG_CATEGORY(LogGLTFSourceImporter, LogVerbosity::Trace);

namespace tinygltf
{
	class Model;
	class Node;
	struct Mesh;
}

namespace Volt
{
	class Mesh;
	class MaterialAsset;
	class MeshInitializer;

	class GLTFSourceImporter final : public SourceAssetImporter
	{
	protected:
		Vector<Ref<Asset>> ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const override;
		SourceAssetFileInformation GetSourceFileInformation(const std::filesystem::path& filepath) const override;

	private:
		void CreateVoltMeshFromGLTFMesh(const tinygltf::Mesh& gltfMesh, const tinygltf::Node& gltfNode, const tinygltf::Model& gltfModel, MeshInitializer& meshInitializer, const Vector<Ref<MaterialAsset>>& materials) const;

		Vector<Ref<Asset>> ImportAsStaticMesh(tinygltf::Model& gltfModel, const MeshSourceImportConfig importConfig, const SourceAssetUserImportData& userData) const;
	};
}
