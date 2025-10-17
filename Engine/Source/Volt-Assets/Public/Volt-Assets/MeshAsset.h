#pragma once

#include "Volt-Assets/Config.h"

#include <Volt-Core/AssetTypes.h>

#include <AssetSystem/Asset.h>

namespace Volt
{
	class Mesh;
	class MeshInitializer;
	class MaterialAsset;

	class VTASSETS_API MeshAsset : public Asset
	{
	public:
		MeshAsset();
		~MeshAsset() override = default;

		void OnDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state) override;

		static AssetType GetStaticType() { return AssetTypes::Mesh; }
		AssetType GetType() override { return GetStaticType(); }
		uint32_t GetVersion() const override { return 2; }

		VT_NODISCARD VT_INLINE Ref<Mesh> GetMesh() const { return m_mesh; }

		void Initialize(const MeshInitializer& meshInitializer, const Vector<Ref<MaterialAsset>>& materials);
		void Initialize(MeshInitializer& meshInitializer, const Vector<AssetHandle>& materials);

	private:
		friend class MeshSerializer;

		Ref<Mesh> m_mesh;
		Vector<AssetHandle> m_materials;

		bool m_isInitialized = false;
	};
}
