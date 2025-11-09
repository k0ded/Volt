#pragma once
#include "Volt-Scene/AssetTypes.h"
#include "Volt-Scene/EntityDescSerializationCommon.h"

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
		void SetupInitialCustomMetadata(CustomAssetMetadataVector& customMetadata) override;
		void Serialize(Archive& archive) override;

		const Buffer& GetEntitySpawnData() const { return m_entitySpawnData; }

		VT_INLINE AssetHandle GetSceneHandle() const { return m_sceneHandle; }
		VT_INLINE EntityID GetEntityID() const { return m_entityID; }

	private:
		friend class EntityDescSerializer;

		AssetHandle m_sceneHandle;
		EntityID m_entityID;

		EntityDescSerialization::SerializationData m_entitySerializationData;
		Buffer m_entitySpawnData;
	};
}
