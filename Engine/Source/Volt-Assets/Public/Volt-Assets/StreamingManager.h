#pragma once

#include "Volt-Assets/Config.h"
#include "Volt-Assets/StreamingInstanceID.h"
#include "Volt-Assets/MeshAsset.h"
#include "Volt-Assets/MaterialAsset.h"

#include "Volt-Renderer/RenderScene/SceneLightData.h"
#include "Volt-Renderer/Texture/EnvironmentTexture.h"

#include <AssetSystem/AssetManager.h>

#include <EventSystem/EventListener.h>
#include <EventSystem/ApplicationEvents.h>

#include <EntitySystem/EntityID.h>
#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/UUID.h>
#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

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
		AssetReference<T> GetAsset(AssetHandle assetHandle);
		
		bool ContainsAsset(AssetHandle assetHandle) const;
		const std::unordered_set<StreamingInstanceID>& GetAssetReferencers(AssetHandle assetHandle) const;

		void SetAssetUpdatedCallback(AssetUpdatedFunc callbackFunc);

	private:
		struct AssetStreamingReference
		{
			AssetReference<T> asset;
			std::unordered_set<StreamingInstanceID> referencers;
		};
		
		VT_PROFILE_DECLARE_MUTEX(std::mutex, m_streamingInstancesMapMutex);

		Map<AssetHandle, AssetStreamingReference> m_assetReferenceFromAssetHandle;
		AssetUpdatedFunc m_callbackFunction;
		AssetType m_assetType;
		UUID64 m_assetUpdatedCallback = 0;
	};

	template<VoltAssetType T>
	bool StreamingInstanceAssetReferenceCounter<T>::ContainsAsset(AssetHandle assetHandle) const
	{
		return m_assetReferenceFromAssetHandle.contains(assetHandle);
	}

	template<VoltAssetType T>
	const std::unordered_set<StreamingInstanceID>& StreamingInstanceAssetReferenceCounter<T>::GetAssetReferencers(AssetHandle assetHandle) const
	{
		VT_ENSURE(m_assetReferenceFromAssetHandle.contains(assetHandle));
		return m_assetReferenceFromAssetHandle.at(assetHandle).referencers;
	}

	template<VoltAssetType T>
	AssetReference<T> StreamingInstanceAssetReferenceCounter<T>::GetAsset(AssetHandle assetHandle)
	{
		return m_assetReferenceFromAssetHandle.at(assetHandle).asset;
	}

	class VTASSETS_API StreamingInstanceMap
	{
	public:
		struct StreamingInstance
		{
			EntityID entityId;
			AssetHandle meshHandle = 0;
			Vector<AssetHandle> materialHandles;
			Ref<ScenePrimitiveData> primitiveData;
		
			// For skylight
			SceneLightDescription sceneLightDescription;
			AssetHandle environmentTextureHandle = 0;
			Ref<SceneLightData> sceneLightData;
		};

		StreamingInstance& Add(StreamingInstanceID id);
		void Erase(StreamingInstanceID id);

		StreamingInstance& Get(StreamingInstanceID id);
		const StreamingInstance& Get(StreamingInstanceID id) const;

		bool Contains(StreamingInstanceID id) const;

	private:
		Map<StreamingInstanceID, StreamingInstance*> m_streamingInstances;
		PagedAtomicArenaAllocator<StreamingInstance, 1024> m_instanceAllocator;
		mutable VT_PROFILE_DECLARE_MUTEX(std::mutex, m_mutex);
	};

	class VTASSETS_API StreamingManager : public SubSystem, public EventListener
	{
	public:
		StreamingManager();
		~StreamingManager();

		void OnPostInitialization() override;

		StreamingInstanceID AddInstance(const StreamingInstanceDescription& description);
		void RemoveInstance(StreamingInstanceID instanceId);

		void InvalidateInstance(StreamingInstanceID instanceId, const StreamingInstanceDescription& description);

		VT_INLINE static StreamingManager& Get() { VT_ENSURE(s_instance); return *s_instance; }
		VT_INLINE static bool IsValid() { return s_instance != nullptr; }

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{B8DF0EF0-A0CC-453D-A4AF-9C8D156F332E}"_guid);

	private:
		void InitializeScenePrimitiveFromInstance(const StreamingInstanceMap::StreamingInstance& instance);
		void InitializeSceneLightDataFromInstance(const StreamingInstanceMap::StreamingInstance& instance);

		void BindToMaterialCompiledDelegate();
		void OnMaterialCompiled(AssetHandle materialHandle);

		bool OnPreRenderEvent(AppPreRenderEvent& event);

		bool IsValidAssetHandle(AssetHandle assetHandle) const;

		StreamingInstanceMap m_streamingInstances;
		StreamingInstanceAssetReferenceCounter<MeshAsset> m_meshReferenceCounter;
		StreamingInstanceAssetReferenceCounter<MaterialAsset> m_materialReferenceCounter;
		StreamingInstanceAssetReferenceCounter<EnvironmentTexture> m_environmentTextureReferenceCounter;

		WorkQueue<AssetHandle, QueueThreadingPolicy::MPSC> m_materialInvalidationQueue;
		std::unordered_set<StreamingInstanceID> m_streamingInstancesToInvalidate;

		inline static StreamingManager* s_instance = nullptr;
	};

	template<VoltAssetType T>
	inline StreamingInstanceAssetReferenceCounter<T>::StreamingInstanceAssetReferenceCounter(AssetType assetType)
		: m_assetType(assetType)
	{
		m_assetUpdatedCallback = g_assetManager->RegisterAssetUpdatedCallback(assetType, [&](AssetHandle assetHandle, AssetChangedState state)
		{
			std::scoped_lock lock{ m_streamingInstancesMapMutex };
			VT_PROFILE_LOCK_MARK(m_streamingInstancesMapMutex);

			auto assetReferenceIt = m_assetReferenceFromAssetHandle.find(assetHandle);

			if (m_callbackFunction && assetReferenceIt != m_assetReferenceFromAssetHandle.end())
			{
				AssetStreamingReference& reference = assetReferenceIt->second;
				VT_ENSURE(reference.asset.IsValid());
				m_callbackFunction(assetHandle, reference.referencers, state);
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
		VT_PROFILE_LOCK_MARK(m_streamingInstancesMapMutex);

		if (!m_assetReferenceFromAssetHandle.contains(assetHandle))
		{
			AssetReference<T> asset;
			g_assetManager->TryGetAsset(assetHandle, asset);
			VT_ENSURE(asset.IsValid());

			m_assetReferenceFromAssetHandle[assetHandle].asset = asset;
		}

		m_assetReferenceFromAssetHandle[assetHandle].referencers.emplace(instanceId);
	}

	template<VoltAssetType T>
	void StreamingInstanceAssetReferenceCounter<T>::RemoveReference(AssetHandle assetHandle, StreamingInstanceID instanceId)
	{
		std::scoped_lock lock{ m_streamingInstancesMapMutex };
		VT_PROFILE_LOCK_MARK(m_streamingInstancesMapMutex);

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
