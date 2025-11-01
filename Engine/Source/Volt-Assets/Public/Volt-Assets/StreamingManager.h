#pragma once

#include "Volt-Assets/Config.h"
#include "Volt-Assets/StreamingInstanceID.h"
#include "Volt-Assets/MeshAsset.h"
#include "Volt-Assets/MaterialAsset.h"

#include "Volt-Renderer/RenderScene/SceneLightData.h"
#include "Volt-Renderer/Texture/EnvironmentTexture.h"

#include <AssetSystem/AssetManager.h>

#include <EntitySystem/EntityID.h>
#include <SubSystem/SubSystem.h>

#include <CoreUtilities/UUID.h>
#include <CoreUtilities/Allocators/PagedArenaAllocator.h>

#include <unordered_set>
#include <functional>

VT_DECLARE_LOG_CATEGORY(LogStreamingManager, LogVerbosity::Trace);

namespace Volt
{
	class ScenePrimitiveData;

	struct StreamingInstanceDescription
	{ 
		EntityID entityId; 
		AssetHandle meshHandle;
		Vector<AssetHandle> materialHandles;
		Ref<ScenePrimitiveData> primitiveData;

		// For skylight
		SceneLightDescription sceneLightDescription;
		AssetHandle environmentTextureHandle;
		Ref<SceneLightData> sceneLightData;
	};

	template<VoltAssetType T>
	class StreamingInstanceAssetReferenceCounter
	{
	public:
		using AssetUpdatedFunc = std::function<void(AssetHandle, const std::unordered_set<StreamingInstanceID>&, AssetChangedState)>;

		StreamingInstanceAssetReferenceCounter(AssetType assetType);
		~StreamingInstanceAssetReferenceCounter();

		void AddReference(AssetHandle assetHandle, StreamingInstanceID instanceId);
		void RemoveReference(AssetHandle assetHandle, StreamingInstanceID instanceId);

		void SetAssetUpdatedCallback(AssetUpdatedFunc callbackFunc);

	private:
		struct AssetStreamingReference
		{
			AssetReference<T> asset;
			std::unordered_set<StreamingInstanceID> referencers;
		};
		
		std::mutex m_streamingInstancesMapMutex;

		Map<AssetHandle, AssetStreamingReference> m_assetReferenceFromAssetHandle;
		AssetUpdatedFunc m_callbackFunction;
		AssetType m_assetType;
		UUID64 m_assetUpdatedCallback = 0;
	};

	class VTASSETS_API StreamingInstanceMap
	{
	public:
		struct StreamingInstance
		{
			EntityID entityId;
			AssetHandle meshHandle;
			Vector<AssetHandle> materialHandles;
			Ref<ScenePrimitiveData> primitiveData;
		
			// For skylight
			SceneLightDescription sceneLightDescription;
			AssetHandle environmentTextureHandle;
			Ref<SceneLightData> sceneLightData;
		};

		StreamingInstance& Add(StreamingInstanceID id);
		void Erase(StreamingInstanceID id);

		StreamingInstance& Get(StreamingInstanceID id);
		const StreamingInstance& Get(StreamingInstanceID id) const;

		bool Contains(StreamingInstanceID id) const;

	private:
		Map<StreamingInstanceID, StreamingInstance*> m_streamingInstances;
		PagedArenaAllocator<StreamingInstance, 1024> m_instanceAllocator;
		mutable std::mutex m_mutex;
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
		void InitializeScenePrimitiveFromInstance(const StreamingInstanceMap::StreamingInstance& instance);
		void InitializeSceneLightDataFromInstance(const StreamingInstanceMap::StreamingInstance& instance);

		StreamingInstanceMap m_streamingInstances;
		StreamingInstanceAssetReferenceCounter<MeshAsset> m_meshReferenceCounter;
		StreamingInstanceAssetReferenceCounter<MaterialAsset> m_materialReferenceCounter;
		StreamingInstanceAssetReferenceCounter<EnvironmentTexture> m_environmentTextureReferenceCounter;

		inline static StreamingManager* s_instance = nullptr;
	};

	template<VoltAssetType T>
	inline StreamingInstanceAssetReferenceCounter<T>::StreamingInstanceAssetReferenceCounter(AssetType assetType)
		: m_assetType(assetType)
	{
		m_assetUpdatedCallback = g_assetManager->RegisterAssetUpdatedCallback(assetType, [&](AssetHandle assetHandle, AssetChangedState state)
		{
			std::scoped_lock lock{ m_streamingInstancesMapMutex };
			if (m_callbackFunction && m_assetReferenceFromAssetHandle.contains(assetHandle))
			{
				m_callbackFunction(assetHandle, m_assetReferenceFromAssetHandle[assetHandle].referencers, state);
			}
		});
	}

	template<VoltAssetType T>
	StreamingInstanceAssetReferenceCounter<T>::~StreamingInstanceAssetReferenceCounter()
	{
		g_assetManager->UnregisterAssetUpdatedCallback(m_assetType, m_assetUpdatedCallback);
	}

	template<VoltAssetType T>
	void StreamingInstanceAssetReferenceCounter<T>::AddReference(AssetHandle assetHandle, StreamingInstanceID instanceId)
	{
		std::scoped_lock lock{ m_streamingInstancesMapMutex };

		if (!m_assetReferenceFromAssetHandle.contains(assetHandle))
		{
			g_assetManager->TryGetAsset(assetHandle, m_assetReferenceFromAssetHandle[assetHandle].asset);
		}

		m_assetReferenceFromAssetHandle[assetHandle].referencers.emplace(instanceId);
	}

	template<VoltAssetType T>
	void StreamingInstanceAssetReferenceCounter<T>::RemoveReference(AssetHandle assetHandle, StreamingInstanceID instanceId)
	{
		std::scoped_lock lock{ m_streamingInstancesMapMutex };

		VT_ENSURE(m_assetReferenceFromAssetHandle.contains(assetHandle));
		VT_ENSURE(m_assetReferenceFromAssetHandle.at(assetHandle).referencers.contains(instanceId));

		m_assetReferenceFromAssetHandle.at(assetHandle).referencers.erase(instanceId);

		if (m_assetReferenceFromAssetHandle.at(assetHandle).referencers.empty())
		{
			m_assetReferenceFromAssetHandle.erase(assetHandle);
		}
	}

	template<VoltAssetType T>
	void StreamingInstanceAssetReferenceCounter<T>::SetAssetUpdatedCallback(AssetUpdatedFunc callbackFunc)
	{
		m_callbackFunction = callbackFunc;
	}
}
