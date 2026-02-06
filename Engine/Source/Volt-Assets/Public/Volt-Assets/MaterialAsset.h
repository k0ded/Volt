#pragma once

#include "Volt-Assets/Config.h"

#include <Volt-Renderer/Texture/Texture2D.h>
#include <Volt-Renderer/Material/RenderMaterial.h>

#include <AssetSystem/AssetTypes.h>
#include <AssetSystem/Asset.h>

namespace Volt
{
	class MaterialGraph;
	class RenderMaterial;

	struct MaterialCustomMetadata
	{
		struct TextureInfo
		{
			AssetHandle handle;
			uint32_t index;

			VT_INLINE friend Archive& operator<<(Archive& archive, TextureInfo& value)
			{
				archive << value.handle;
				archive << value.index;
				return archive;
			}
		};

		static bool IsForAssetType(const AssetType& assetType) { return assetType->GetGUID() == AssetTypes::Material->GetGUID(); }
		
		Vector<TextureInfo> textureReferences;

		VT_INLINE friend Archive& operator<<(Archive& archive, MaterialCustomMetadata& value)
		{
			archive << value.textureReferences;
			return archive;
		}
	};

	class VTASSETS_API MaterialAsset : public Asset
	{
	public:
		MaterialAsset();
		~MaterialAsset() override = default;

		VT_NODISCARD VT_INLINE Ref<MaterialGraph> GetMaterialGraph() const { return m_graph; }
		VT_NODISCARD VT_INLINE Ref<RenderMaterial> GetRenderMaterial() const { return m_renderMaterial; }
		VT_NODISCARD VT_INLINE MaterialBlendMode GetMaterialBlendMode() const { return m_materialBlendMode; }
		VT_NODISCARD VT_INLINE bool GetIsDoubleSided() const { return m_isDoubleSided; }

		static AssetType GetStaticType() { return AssetTypes::Material; }
		AssetType GetType() const override { return GetStaticType(); };
		void OnAssetDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state) override;
		void Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata) override;
		void OnPreSave(CustomAssetMetadata& customMetadata) override;
		void GatherAssetDependencies(AssetDependencyGatherContext& gatherContext, ReadOnlyAssetMetadata assetMetadata) override;

		VT_INLINE void SetMaterialBlendMode(MaterialBlendMode materialBlendMode) { m_materialBlendMode = materialBlendMode; }
		VT_INLINE void SetIsDoubleSided(bool isDoubleSided) { m_isDoubleSided = isDoubleSided; }

	private:
		Ref<MaterialGraph> m_graph;
		Ref<RenderMaterial> m_renderMaterial;
	
		MaterialBlendMode m_materialBlendMode = MaterialBlendMode::Opaque;
		bool m_isDoubleSided = false;

		Vector<AssetReference<Texture2D>> m_referencedTextures;
	};
}
