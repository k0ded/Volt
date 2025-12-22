#pragma once
#include "Volt-Scene/AssetTypes.h"
#include "Volt-Scene/EntityDescSerializationCommon.h"
#include "Volt-Scene/Scene.h"

#include <AssetSystem/Asset.h>

#include <EntitySystem/EntityID.h>

namespace Volt
{
	class VTS_API EntityDesc : public Asset
	{
	public:
		EntityDesc() = default;
		EntityDesc( EntityID entityID, AssetHandle sceneHandle);
		~EntityDesc() override = default;

		static AssetType GetStaticType() { return AssetTypes::EntityDesc; }
		AssetType GetType() const override { return GetStaticType(); }
		uint32_t GetVersion() const override { return 1; }
		void OnPreSave(CustomAssetMetadata& customMetadata) override;
		void Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata) override;

		const Buffer& GetEntitySpawnData() const { return m_entitySpawnData; }

		VT_INLINE const EntityDescSerialization::SerializationData& GetSerializationData() const { return m_entitySerializationData; }
		VT_INLINE AssetHandle GetSceneHandle() const { return m_sceneHandle; }
		VT_INLINE EntityID GetEntityID() const { return m_entityID; }
		VT_INLINE void AssignOwnerScene(AssetReference<Scene> ownerScene) { m_ownerScene = ownerScene; }

	private:
		AssetHandle m_sceneHandle = 0;
		EntityID m_entityID = 0;

		EntityDescSerialization::SerializationData m_entitySerializationData;
		AssetReference<Scene> m_ownerScene;
		Buffer m_entitySpawnData;
	};
}
