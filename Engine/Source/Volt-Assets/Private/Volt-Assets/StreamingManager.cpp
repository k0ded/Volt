#include "vtassetspch.h"

#include "Volt-Assets/StreamingManager.h"
#include "Volt-Assets/MeshAsset.h"
#include "Volt-Assets/MaterialAsset.h"

#include <Volt-Renderer/RenderScene/ScenePrimitiveData.h>
#include <Volt-Renderer/Renderer.h>
#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Texture/EnvironmentTexture.h>

#include <Volt-Core/AssetTypes.h>
#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <AssetSystem/AssetManager.h>

VT_DEFINE_LOG_CATEGORY(LogStreamingManager);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(StreamingManager, Default, Engine, 0);

	static ConsoleVariable<int32_t> s_logStreamingManagerUpdates("r.StreamingManager.LogUpdates", 0, "Whether or not to log Streaming Manager updates");

	StreamingManager::StreamingManager()
		: m_meshReferenceCounter(AssetTypes::Mesh),
		m_materialReferenceCounter(AssetTypes::Material),
		m_environmentTextureReferenceCounter(AssetTypes::EnvironmentTexture)
	{
		VT_ENSURE(!s_instance);

		s_instance = this;

		m_meshReferenceCounter.SetAssetUpdatedCallback([&](AssetHandle meshHandle, const std::unordered_set<StreamingInstanceID>& streamingInstances, AssetChangedState state)
		{
			if (state == AssetChangedState::Loaded)
			{
				for (const auto& instanceId : streamingInstances)
				{
					const auto& instance = m_streamingInstances.Get(instanceId);
					InitializeScenePrimitiveFromInstance(instance);
				}
			}
		});

		m_materialReferenceCounter.SetAssetUpdatedCallback([&](AssetHandle meshHandle, const std::unordered_set<StreamingInstanceID>& streamingInstances, AssetChangedState state)
		{
			if (state == AssetChangedState::Loaded)
			{
				for (const auto& instanceId : streamingInstances)
				{
					const auto& instance = m_streamingInstances.Get(instanceId);
					InitializeScenePrimitiveFromInstance(instance);
				}
			}
		});

		m_environmentTextureReferenceCounter.SetAssetUpdatedCallback([&](AssetHandle textureHandle, const std::unordered_set<StreamingInstanceID>& streamingInstances, AssetChangedState state)
		{
			if (state == AssetChangedState::Loaded)
			{
				for (const auto& instanceId : streamingInstances)
				{
					const auto& instance = m_streamingInstances.Get(instanceId);
					InitializeSceneLightDataFromInstance(instance);
				}
			}
		});
	}

	StreamingManager::~StreamingManager()
	{
		s_instance = nullptr;
	}

	StreamingInstanceID StreamingManager::AddInstance(const StreamingInstanceDescription& description)
	{
		StreamingInstanceID newId;

		StreamingInstanceMap::StreamingInstance& instance = m_streamingInstances.Add(newId);
		
		// It's a primitive
		if (description.primitiveData)
		{
			instance.entityId = description.entityId;
			instance.meshHandle = description.meshHandle;
			instance.materialHandles = description.materialHandles;
			instance.primitiveData = description.primitiveData;

			for (const auto& materialHandle : description.materialHandles)
			{
				m_materialReferenceCounter.AddReference(materialHandle, newId);
			}

			m_meshReferenceCounter.AddReference(description.meshHandle, newId);

			if (s_logStreamingManagerUpdates.GetValue())
			{
				VT_LOGC(Trace, LogStreamingManager, "Added a new instance linked to entity {} with mesh {} and gave it ID {}", description.entityId, description.meshHandle, newId);
			}

			InitializeScenePrimitiveFromInstance(m_streamingInstances.Get(newId));
		}
		// It's a skylight
		else if (description.sceneLightData)
		{
			VT_ENSURE_MSG(description.sceneLightDescription.lightType == SceneLightType::Sky, "Only skylights should be added to the streaming manager!");

			instance.environmentTextureHandle = description.environmentTextureHandle;
			instance.sceneLightData = description.sceneLightData;
			instance.sceneLightDescription = description.sceneLightDescription;
			instance.entityId = description.entityId;

			m_environmentTextureReferenceCounter.AddReference(description.environmentTextureHandle, newId);

			if (s_logStreamingManagerUpdates.GetValue())
			{
				VT_LOGC(Trace, LogStreamingManager, "Added a new instance linked to entity {} with environment texture {} and gave it ID {}", description.entityId, description.environmentTextureHandle, newId);
			}
		}
		else
		{
			VT_ENSURE(false);
		}

		return newId;
	}

	void StreamingManager::RemoveInstance(StreamingInstanceID instanceId)
	{
		const auto& instance = m_streamingInstances.Get(instanceId);

		if (s_logStreamingManagerUpdates.GetValue())
		{
			VT_LOGC(Trace, LogStreamingManager, "Removed instance with ID {} which is linked to entity {}", instanceId, instance.entityId);
		}

		if (instance.primitiveData)
		{
			m_meshReferenceCounter.RemoveReference(instance.meshHandle, instanceId);

			for (const auto& materialHandle : instance.materialHandles)
			{
				m_materialReferenceCounter.RemoveReference(materialHandle, instanceId);
			}
		}
		else if (instance.sceneLightData)
		{
			m_environmentTextureReferenceCounter.RemoveReference(instance.environmentTextureHandle, instanceId);
		}

		if (m_streamingInstances.Contains(instanceId))
		{
			m_streamingInstances.Erase(instanceId);
		}
	}

	void StreamingManager::InvalidateInstance(StreamingInstanceID instanceId, const StreamingInstanceDescription& description)
	{
		if (!m_streamingInstances.Contains(instanceId))
		{
			return;
		}

		auto& streamingInstance = m_streamingInstances.Get(instanceId);

		if (streamingInstance.primitiveData)
		{
			for (const auto& materialHandle : streamingInstance.materialHandles)
			{
				m_materialReferenceCounter.RemoveReference(materialHandle, instanceId);
			}

			m_meshReferenceCounter.RemoveReference(streamingInstance.meshHandle, instanceId);

			for (const auto& materialHandle : description.materialHandles)
			{
				m_materialReferenceCounter.AddReference(materialHandle, instanceId);
			}

			m_meshReferenceCounter.AddReference(description.meshHandle, instanceId);

			streamingInstance.meshHandle = description.meshHandle;
			streamingInstance.materialHandles = description.materialHandles;

			InitializeScenePrimitiveFromInstance(streamingInstance);

			if (s_logStreamingManagerUpdates.GetValue())
			{
				VT_LOGC(Trace, LogStreamingManager, "Invalidated instance with ID {} linked to entity {} and has mesh {}", instanceId, streamingInstance.entityId, streamingInstance.meshHandle);
			}
		}
		else if (streamingInstance.sceneLightData)
		{
			m_environmentTextureReferenceCounter.RemoveReference(streamingInstance.environmentTextureHandle, instanceId);
			m_environmentTextureReferenceCounter.AddReference(description.environmentTextureHandle, instanceId);

			streamingInstance.environmentTextureHandle = description.environmentTextureHandle;

			InitializeSceneLightDataFromInstance(streamingInstance);

			if (s_logStreamingManagerUpdates.GetValue())
			{
				VT_LOGC(Trace, LogStreamingManager, "Invalidated instance with ID {} linked to entity {} and environment texture {}", instanceId, streamingInstance.entityId, streamingInstance.environmentTextureHandle);
			}
		}
	}

	void StreamingManager::InitializeScenePrimitiveFromInstance(const StreamingInstanceMap::StreamingInstance& instance)
	{
		Ref<MeshAsset> meshAsset = AssetManager::QueueAsset<MeshAsset>(instance.meshHandle);
		Ref<Mesh> mesh;

		if (meshAsset && meshAsset->IsValid())
		{
			mesh = meshAsset->GetMesh();
		}
		else
		{
			mesh = Renderer::GetDefaultResources().defaultMesh;
		}

		ScenePrimitiveDescription primitiveDescription;
		primitiveDescription.primitiveMesh = mesh;

		for (const auto& materialHandle : instance.materialHandles)
		{
			if (materialHandle == Asset::Null())
			{
				continue;
			}

			Ref<MaterialAsset> materialAsset = AssetManager::QueueAsset<MaterialAsset>(materialHandle);
			Ref<RenderMaterial> renderMaterial;
			
			if (materialAsset && materialAsset->IsValid())
			{
				renderMaterial = materialAsset->GetRenderMaterial();
			}
			else
			{
				renderMaterial = Renderer::GetDefaultResources().defaultMaterial;
			}

			primitiveDescription.materials.emplace_back(renderMaterial);
		}

		const auto& meshMaterialTable = mesh->GetMaterialTable();
		const size_t meshMaterialTableSize = static_cast<size_t>(primitiveDescription.materials.size());

		if (meshMaterialTableSize < meshMaterialTable.GetSize())
		{
			const size_t prevSize = primitiveDescription.materials.size();

			primitiveDescription.materials.resize(meshMaterialTable.GetSize());

			for (size_t i = prevSize; i < meshMaterialTableSize; i++)
			{
				primitiveDescription.materials[i] = meshMaterialTable.GetMaterial(static_cast<uint32_t>(i));
			}
		}

		instance.primitiveData->InitializeFromDescription(primitiveDescription);
	}

	void StreamingManager::InitializeSceneLightDataFromInstance(const StreamingInstanceMap::StreamingInstance& instance)
	{
		Ref<EnvironmentTexture> environmentTexture = AssetManager::QueueAsset<EnvironmentTexture>(instance.environmentTextureHandle);

		SceneLightDescription lightDescription = instance.sceneLightDescription;
		
		if (environmentTexture && environmentTexture->IsValid())
		{
			lightDescription.diffuseIBL = environmentTexture->GetDiffuseImage();
			lightDescription.specularIBL = environmentTexture->GetSpecularImage();
		}
		else
		{
			lightDescription.diffuseIBL = Renderer::GetDefaultResources().blackCubeTexture;
			lightDescription.specularIBL = Renderer::GetDefaultResources().blackCubeTexture;
		}

		instance.sceneLightData->InitializeFromDescription(lightDescription);
	}

	StreamingInstanceAssetReferenceCounter::StreamingInstanceAssetReferenceCounter(AssetType assetType)
		: m_assetType(assetType)
	{
		m_assetUpdatedCallback = AssetManager::RegisterAssetUpdatedCallback(assetType, [&](AssetHandle assetHandle, AssetChangedState state)
		{
			std::scoped_lock lock{ m_streamingInstancesMapMutex };
			if (m_callbackFunction && m_streamingInstancesFromAssetHandle.contains(assetHandle))
			{
				m_callbackFunction(assetHandle, m_streamingInstancesFromAssetHandle[assetHandle], state);
			}
		});
	}

	StreamingInstanceAssetReferenceCounter::~StreamingInstanceAssetReferenceCounter()
	{
		AssetManager::UnregisterAssetUpdatedCallback(m_assetType, m_assetUpdatedCallback);
	}

	void StreamingInstanceAssetReferenceCounter::AddReference(AssetHandle assetHandle, StreamingInstanceID instanceId)
	{
		std::scoped_lock lock{ m_streamingInstancesMapMutex };
		m_streamingInstancesFromAssetHandle[assetHandle].emplace(instanceId);
	}
	
	void StreamingInstanceAssetReferenceCounter::RemoveReference(AssetHandle assetHandle, StreamingInstanceID instanceId)
	{
		std::scoped_lock lock{ m_streamingInstancesMapMutex };

		VT_ENSURE(m_streamingInstancesFromAssetHandle.contains(assetHandle));
		VT_ENSURE(m_streamingInstancesFromAssetHandle.at(assetHandle).contains(instanceId));
	
		m_streamingInstancesFromAssetHandle.at(assetHandle).erase(instanceId);
	}

	void StreamingInstanceAssetReferenceCounter::SetAssetUpdatedCallback(AssetUpdatedFunc callbackFunc)
	{
		m_callbackFunction = callbackFunc;
	}

	StreamingInstanceMap::StreamingInstance& StreamingInstanceMap::Get(StreamingInstanceID id)
	{
		std::scoped_lock lock(m_mutex);
		VT_ENSURE(m_streamingInstances.contains(id));
		return *m_streamingInstances.at(id);
	}
	const StreamingInstanceMap::StreamingInstance& StreamingInstanceMap::Get(StreamingInstanceID id) const
	{
		std::scoped_lock lock(m_mutex);
		VT_ENSURE(m_streamingInstances.contains(id));
		return *m_streamingInstances.at(id);
	}

	bool StreamingInstanceMap::Contains(StreamingInstanceID id) const
	{
		std::scoped_lock lock(m_mutex);
		return m_streamingInstances.contains(id);
	}

	StreamingInstanceMap::StreamingInstance& StreamingInstanceMap::Add(StreamingInstanceID id)
	{
		std::scoped_lock lock(m_mutex);
		StreamingInstance* newInstance = m_instanceAllocator.Allocate();
		m_streamingInstances[id] = newInstance;

		return *newInstance;
	}

	void StreamingInstanceMap::Erase(StreamingInstanceID id)
	{
		std::scoped_lock lock(m_mutex);

		StreamingInstance* instance = m_streamingInstances.at(id);
		m_streamingInstances.erase(id);

		m_instanceAllocator.Free(instance);
	}
}
