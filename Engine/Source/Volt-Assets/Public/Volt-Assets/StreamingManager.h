#pragma once

#include "Volt-Assets/Config.h"
#include "Volt-Assets/StreamingInstanceID.h"

#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetHandle.h>

#include <EntitySystem/EntityID.h>
#include <SubSystem/SubSystem.h>

#include <CoreUtilities/UUID.h>
#include <CoreUtilities/Containers/VectorVariants.h>

#include <unordered_set>
#include <functional>

namespace Volt
{
	class ScenePrimitiveData;

	struct StreamingInstanceDescription
	{ 
		EntityID entityId; 
		AssetHandle meshHandle;
		Vector<AssetHandle> materialHandles;

		Ref<ScenePrimitiveData> primitiveData;
	};

	class VTASSETS_API StreamingInstanceAssetReferenceCounter
	{
	public:
		using AssetUpdatedFunc = std::function<void(AssetHandle, const std::unordered_set<StreamingInstanceID>&, AssetChangedState)>;

		StreamingInstanceAssetReferenceCounter(AssetType assetType);
		~StreamingInstanceAssetReferenceCounter();

		void AddReference(AssetHandle assetHandle, StreamingInstanceID instanceId);
		void RemoveReference(AssetHandle assetHandle, StreamingInstanceID instanceId);

		void SetAssetUpdatedCallback(AssetUpdatedFunc callbackFunc);

	private:
		vt::map<AssetHandle, std::unordered_set<StreamingInstanceID>> m_streamingInstancesFromAssetHandle;
		AssetUpdatedFunc m_callbackFunction;
		AssetType m_assetType;
		UUID64 m_assetUpdatedCallback = 0;
	};

	class VTASSETS_API StreamingManager : public SubSystem
	{
	public:
		StreamingManager();
		~StreamingManager();

		StreamingInstanceID AddInstance(const StreamingInstanceDescription& description);
		void RemoveInstance(StreamingInstanceID instanceId);

		void InvalidateInstance(StreamingInstanceID instanceId, const StreamingInstanceDescription& description);

		VT_INLINE static StreamingManager& Get() { VT_ENSURE(s_instance); return *s_instance; }

		VT_DECLARE_SUBSYSTEM("{B8DF0EF0-A0CC-453D-A4AF-9C8D156F332E}"_guid);

	private:
		struct StreamingInstance
		{
			EntityID entityId;
			AssetHandle meshHandle;
			Vector<AssetHandle> materialHandles;
			Ref<ScenePrimitiveData> primitiveData;
		};

		void InitializeScenePrimitiveFromInstance(const StreamingInstance& instance);

		vt::map<StreamingInstanceID, StreamingInstance> m_streamingInstances;
		StreamingInstanceAssetReferenceCounter m_meshReferenceCounter;
		StreamingInstanceAssetReferenceCounter m_materialReferenceCounter;

		inline static StreamingManager* s_instance = nullptr;
	};
}
