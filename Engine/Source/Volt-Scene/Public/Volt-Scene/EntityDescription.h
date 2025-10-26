#pragma once
#include "Volt-Scene/AssetTypes.h"

#include <AssetSystem/Asset_New.h>

#include <EntitySystem/EntityID.h>

namespace Volt
{
	class VTS_API EntityDesc : public Asset_New
	{
	public:
		EntityDesc() = default;
		EntityDesc( EntityID entityID, AssetHandle sceneHandle);
		~EntityDesc() override = default;

		static AssetType GetStaticType() { return AssetTypes::EntityDesc; }
		AssetType GetType() const override { return GetStaticType(); }
		uint32_t GetVersion() const override { return 1; }
		void SetupInitialCustomMetadata(CustomAssetMetadataVector& customMetadata) override;

		const Buffer& GetEntitySpawnData() const { return m_entitySpawnData; }

		VT_INLINE AssetHandle GetSceneHandle() const { return m_sceneHandle; }
		VT_INLINE EntityID GetEntityID() const { return m_entityID; }

	private:
		friend class EntityDescSerializer;

		AssetHandle m_sceneHandle;
		EntityID m_entityID;

		Buffer m_entitySpawnData;
	};
}
