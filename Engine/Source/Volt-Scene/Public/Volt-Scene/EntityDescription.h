#pragma once
#include <AssetSystem/Asset.h>

#include <EntitySystem/EntityID.h>


namespace Volt
{
	class EntityDesc : public Asset
	{
	public:
		EntityDesc() = delete;
		EntityDesc( EntityID entityID, AssetHandle sceneHandle);
		~EntityDesc() override = default;

		__forceinline AssetHandle GetSceneHandle() { return m_sceneHandle; }
		__forceinline EntityID GetEntityID() { return m_entityID; }

	private:
		friend class EntityDescSerializer;

		AssetHandle m_sceneHandle;
		EntityID m_entityID;
	};
}
