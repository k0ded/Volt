#pragma once
#include "Volt-Scene/AssetTypes.h"

#include <AssetSystem/Asset.h>

#include <EntitySystem/EntityID.h>



namespace Volt
{
	class VTS_API EntityDesc : public Asset
	{
	public:
		EntityDesc() = delete;
		EntityDesc( EntityID entityID, AssetHandle sceneHandle);
		~EntityDesc() override = default;

		static AssetType GetStaticType() { return AssetTypes::EntityDesc; }
		AssetType GetType() override { return GetStaticType(); }
		uint32_t GetVersion() const override { return 1; }

		__forceinline AssetHandle GetSceneHandle() { return m_sceneHandle; }
		__forceinline EntityID GetEntityID() { return m_entityID; }

	private:
		friend class EntityDescSerializer;

		AssetHandle m_sceneHandle;
		EntityID m_entityID;
	};
}
