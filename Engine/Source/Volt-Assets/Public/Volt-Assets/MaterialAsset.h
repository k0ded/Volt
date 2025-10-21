#pragma once

#include "Volt-Assets/Config.h"

#include <AssetSystem/AssetTypes.h>

#include <AssetSystem/Asset.h>

namespace Volt
{
	class MaterialGraph;
	class RenderMaterial;

	class VTASSETS_API MaterialAsset : public Asset
	{
	public:
		MaterialAsset();
		~MaterialAsset() override = default;

		VT_NODISCARD VT_INLINE Ref<MaterialGraph> GetMaterialGraph() const { return m_graph; }
		VT_NODISCARD VT_INLINE Ref<RenderMaterial> GetRenderMaterial() const { return m_renderMaterial; }
		VT_NODISCARD VT_INLINE const std::string& GetName() const { return assetName; }

		static AssetType GetStaticType() { return AssetTypes::Material; }
		AssetType GetType() override { return GetStaticType(); };
		void OnDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state) override;

	private:
		friend class MaterialSerializer;

		Ref<MaterialGraph> m_graph;
		Ref<RenderMaterial> m_renderMaterial;
	};
}
