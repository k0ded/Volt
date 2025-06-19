#pragma once

#include "Volt-Assets/Config.h"

#include <Volt-Core/AssetTypes.h>

#include <AssetSystem/Asset.h>

namespace Volt
{
	class Mesh;

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

	private:
		friend class MeshSerializer;
		friend class GLTFSourceImporter;
		friend class FbxSourceImporter;

		void FinalizeDeserialization();

		Ref<Mesh> m_mesh;
		Map<uint32_t, AssetHandle> m_materials;
	};
}
