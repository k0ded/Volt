#pragma once

#include "Volt-Assets/Config.h"
#include "Volt-Assets/MaterialAsset.h"

#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetTypes.h>
#include <AssetSystem/CustomAssetMetadataRegistry.h>

namespace Volt
{
	class Mesh;
	class MeshInitializer;
	class MaterialAsset;

	struct MeshCustomMetadata
	{
		static bool IsForAssetType(const AssetType& assetType) { return assetType->GetGUID() == AssetTypes::Mesh->GetGUID(); }
		
		Vector<AssetHandle> materials;

		VT_INLINE friend Archive& operator<<(Archive& archive, MeshCustomMetadata& value)
		{
			archive << value.materials;
			return archive;
		}
	};

	class VTASSETS_API MeshAsset : public Asset
	{
	public:
		MeshAsset();
		~MeshAsset() override = default;

		void OnAssetDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state) override;

		static AssetType GetStaticType() { return AssetTypes::Mesh; }
		AssetType GetType() const override { return GetStaticType(); }
		uint32_t GetVersion() const override { return 2; }
		void OnPreSave(CustomAssetMetadata& customMetadata) override;
		void Serialize(Archive& archive) override;

		VT_NODISCARD VT_INLINE Ref<Mesh> GetMesh() const { return m_mesh; }

		void Initialize(const MeshInitializer& meshInitializer, const Vector<AssetReference<MaterialAsset>>& materials);
		void Initialize(MeshInitializer& meshInitializer, const Vector<AssetHandle>& materials);

	private:
		Ref<Mesh> m_mesh;
		Vector<AssetHandle> m_materials;
		Vector<AssetReference<MaterialAsset>> m_materialReferences;

		bool m_isInitialized = false;
	};
}
