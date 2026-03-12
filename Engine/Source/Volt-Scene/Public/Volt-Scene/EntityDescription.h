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

		VT_INLINE const EntityDescSerialization::ComponentData& GetComponentData() const { return m_componentData; }
		VT_INLINE AssetHandle GetSceneHandle() const { return m_sceneHandle; }
		VT_INLINE EntityID GetEntityID() const { return m_entityID; }

		// update the component data 
		// if the owning scene is not loaded or does not contain the entity, this will return false and do nothing
		bool UpdateComponentData();
		// update the component data accordingly for the specified component
		// if the owning scene is not loaded or does not contain the entity, this will return false and do nothing
		bool UpdateComponentData(VoltGUID changedComponent);

	private:
		AssetHandle m_sceneHandle = 0;
		EntityID m_entityID = 0;

		EntityDescSerialization::ComponentData m_componentData;
	};
}
