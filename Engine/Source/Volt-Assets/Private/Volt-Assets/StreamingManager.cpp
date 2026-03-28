#include "vtassetspch.h"

#include "Volt-Assets/StreamingManager.h"
#include "Volt-Assets/MeshAsset.h"
#include "Volt-Assets/MaterialAsset.h"
#include "Volt-Assets/MaterialCompilerSubSystem.h"

#include <CoreModule/Console/ConsoleVariableRegistry.h>

#include <Volt-Renderer/RenderScene/ScenePrimitiveData.h>
#include <Volt-Renderer/Renderer.h>
#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Texture/EnvironmentTexture.h>

#include <AssetSystem/AssetTypes.h>
#include <AssetSystem/AssetManager.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/Profiling/Profiling.h>

VT_DEFINE_LOG_CATEGORY(LogStreamingManager);

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(StreamingManager, Default, Engine);

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
				m_streamingInstancesToInvalidate.insert(streamingInstances.begin(), streamingInstances.end());
			}
		});

		m_materialReferenceCounter.SetAssetUpdatedCallback([&](AssetHandle meshHandle, const std::unordered_set<StreamingInstanceID>& streamingInstances, AssetChangedState state)
		{
			if (state == AssetChangedState::Loaded)
			{
				m_streamingInstancesToInvalidate.insert(streamingInstances.begin(), streamingInstances.end());
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

		m_materialInvalidationQueue.Allocate(1024);

		RegisterListener<AppPreRenderEvent>(VT_BIND_EVENT_FN(StreamingManager::OnPreRenderEvent));
	}

	StreamingManager::~StreamingManager()
	{
		s_instance = nullptr;
	}

	void StreamingManager::OnPostInitialization()
	{
		BindToMaterialCompiledDelegate();
	}

	StreamingInstanceID StreamingManager::AddInstance(const StreamingInstanceDescription& description)
	{
		StreamingInstanceID newId;

		// Make sure id is not in use.
		while (m_streamingInstances.Contains(newId))
		{
			newId = {};
		}

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
				if (IsValidAssetHandle(materialHandle))
				{
					m_materialReferenceCounter.AddReference(materialHandle, newId);
				}
			}

			const bool isValidMeshAsset = IsValidAssetHandle(description.meshHandle);

			if (isValidMeshAsset)
			{
				m_meshReferenceCounter.AddReference(description.meshHandle, newId);
			}

			if (s_logStreamingManagerUpdates.GetValue())
			{
				VT_LOGC(Trace, LogStreamingManager, "Added a new instance linked to entity {} with mesh {} and gave it ID {}", description.entityId, description.meshHandle, newId);
			}

			if (isValidMeshAsset)
			{
				InitializeScenePrimitiveFromInstance(m_streamingInstances.Get(newId));
			}
		}
		// It's a skylight
		else if (description.sceneLightData)
		{
			VT_ENSURE_MSG(description.sceneLightDescription.lightType == SceneLightType::Sky, "Only skylights should be added to the streaming manager!");

			instance.environmentTextureHandle = description.environmentTextureHandle;
			instance.sceneLightData = description.sceneLightData;
			instance.sceneLightDescription = description.sceneLightDescription;
			instance.entityId = description.entityId;

			const bool environmentTextureIsValidAsset = IsValidAssetHandle(description.environmentTextureHandle);

			if (environmentTextureIsValidAsset)
			{
				m_environmentTextureReferenceCounter.AddReference(description.environmentTextureHandle, newId);
			}

			if (s_logStreamingManagerUpdates.GetValue())
			{
				VT_LOGC(Trace, LogStreamingManager, "Added a new instance linked to entity {} with environment texture {} and gave it ID {}", description.entityId, description.environmentTextureHandle, newId);
			}

			if (environmentTextureIsValidAsset)
			{
				InitializeSceneLightDataFromInstance(m_streamingInstances.Get(newId));
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
		if (!m_streamingInstances.Contains(instanceId))
		{
			return;
		}

		const auto& instance = m_streamingInstances.Get(instanceId);

		if (s_logStreamingManagerUpdates.GetValue())
		{
			VT_LOGC(Trace, LogStreamingManager, "Removed instance with ID {} which is linked to entity {}", instanceId, instance.entityId);
		}

		if (instance.primitiveData)
		{
			if (IsValidAssetHandle(instance.meshHandle))
			{
				m_meshReferenceCounter.RemoveReference(instance.meshHandle, instanceId);
			}

			for (const auto& materialHandle : instance.materialHandles)
			{
				if (IsValidAssetHandle(materialHandle))
				{
					m_materialReferenceCounter.RemoveReference(materialHandle, instanceId);
				}
			}
		}
		else if (instance.sceneLightData)
		{
			if (IsValidAssetHandle(instance.environmentTextureHandle))
			{
				m_environmentTextureReferenceCounter.RemoveReference(instance.environmentTextureHandle, instanceId);
			}
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
			if (streamingInstance.meshHandle != description.meshHandle)
			{
				if (IsValidAssetHandle(streamingInstance.meshHandle))
				{
					m_meshReferenceCounter.RemoveReference(streamingInstance.meshHandle, instanceId);
				}

				if (IsValidAssetHandle(description.meshHandle))
				{
					m_meshReferenceCounter.AddReference(description.meshHandle, instanceId);
				}
			}

			std::unordered_set<AssetHandle> newMaterialsSet(description.materialHandles.begin(), description.materialHandles.end());
			std::unordered_set<AssetHandle> oldMaterialsSet(streamingInstance.materialHandles.begin(), streamingInstance.materialHandles.end());

			Vector<AssetHandle> materialsToAdd;
			Vector<AssetHandle> materialsToRemove;

			for (const AssetHandle& materialHandle : description.materialHandles)
			{
				if (!oldMaterialsSet.count(materialHandle))
				{
					materialsToAdd.emplace_back(materialHandle);
				}
			}

			for (const AssetHandle& materialHandle : streamingInstance.materialHandles)
			{
				if (!newMaterialsSet.count(materialHandle))
				{
					materialsToRemove.emplace_back(materialHandle);
				}
			}

			for (const auto& materialHandle : materialsToAdd)
			{
				if (IsValidAssetHandle(materialHandle))
				{
					m_materialReferenceCounter.AddReference(materialHandle, instanceId);
				}
			}

			for (const auto& materialHandle : materialsToRemove)
			{
				if (IsValidAssetHandle(materialHandle))
				{
					m_materialReferenceCounter.RemoveReference(materialHandle, instanceId);
				}
			}

			streamingInstance.meshHandle = description.meshHandle;
			streamingInstance.materialHandles = description.materialHandles;

			if (streamingInstance.meshHandle != Asset::Null())
			{
				InitializeScenePrimitiveFromInstance(streamingInstance);
			}

			if (s_logStreamingManagerUpdates.GetValue())
			{
				VT_LOGC(Trace, LogStreamingManager, "Invalidated instance with ID {} linked to entity {} and has mesh {}", instanceId, streamingInstance.entityId, streamingInstance.meshHandle);
			}
		}
		else if (streamingInstance.sceneLightData)
		{
			if (description.environmentTextureHandle != streamingInstance.environmentTextureHandle)
			{
				if (IsValidAssetHandle(description.environmentTextureHandle))
				{
					m_environmentTextureReferenceCounter.AddReference(description.environmentTextureHandle, instanceId);
				}

				if (IsValidAssetHandle(streamingInstance.environmentTextureHandle))
				{
					m_environmentTextureReferenceCounter.RemoveReference(streamingInstance.environmentTextureHandle, instanceId);
				}

				streamingInstance.environmentTextureHandle = description.environmentTextureHandle;
			}

			streamingInstance.sceneLightDescription = description.sceneLightDescription;
			InitializeSceneLightDataFromInstance(streamingInstance);

			if (s_logStreamingManagerUpdates.GetValue())
			{
				VT_LOGC(Trace, LogStreamingManager, "Invalidated instance with ID {} linked to entity {} and environment texture {}", instanceId, streamingInstance.entityId, streamingInstance.environmentTextureHandle);
			}
		}
	}

	void StreamingManager::InitializeScenePrimitiveFromInstance(const StreamingInstanceMap::StreamingInstance& instance)
	{
		Ref<Mesh> mesh;

		AssetReference<MeshAsset> meshAsset;

		if (g_assetManager->TryGetAsset<MeshAsset>(instance.meshHandle, meshAsset))
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
			Ref<RenderMaterial> renderMaterial;
			
			AssetReference<MaterialAsset> materialAsset;

			if (IsValidAssetHandle(materialHandle))
			{
				if (g_assetManager->TryGetAsset(materialHandle, materialAsset))
				{
					renderMaterial = materialAsset->GetRenderMaterial();
				}
			}

			// Material handle was null, or material was not loaded/found.
			if (renderMaterial == nullptr)
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
		SceneLightDescription lightDescription = instance.sceneLightDescription;
		
		AssetReference<EnvironmentTexture> environmentTexture;
		if (g_assetManager->TryGetAsset(instance.environmentTextureHandle, environmentTexture))
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

	void StreamingManager::BindToMaterialCompiledDelegate()
	{
		if (MaterialCompilerSubSystem* compilerSubSystem = SubSystemManager::GetSubSystem<MaterialCompilerSubSystem>(); compilerSubSystem != nullptr)
		{
			compilerSubSystem->GetMaterialCompiledDelegate().BindRaw(this, &StreamingManager::OnMaterialCompiled);
		}
	}

	void StreamingManager::OnMaterialCompiled(AssetHandle materialHandle)
	{
		// Since the delegate call may come from any worker,
		// we queue the invalidation, and perform it later in the OnPreRenderEvent.
		m_materialInvalidationQueue.Emplace(materialHandle);
	}

	bool StreamingManager::OnPreRenderEvent(AppPreRenderEvent& event)
	{
		VT_PROFILE_FUNCTION();

		AssetHandle materialHandle;
		while (m_materialInvalidationQueue.Pop(materialHandle))
		{
			if (m_materialReferenceCounter.ContainsAsset(materialHandle))
			{
				const std::unordered_set<StreamingInstanceID>& referencers = m_materialReferenceCounter.GetAssetReferencers(materialHandle);
				m_streamingInstancesToInvalidate.insert(referencers.begin(), referencers.end());
			}
		}

		for (const StreamingInstanceID& referencerId : m_streamingInstancesToInvalidate)
		{
			const auto& instance = m_streamingInstances.Get(referencerId);
			InitializeScenePrimitiveFromInstance(instance);
		}
		m_streamingInstancesToInvalidate.clear();

		return false;
	}

	bool StreamingManager::IsValidAssetHandle(AssetHandle assetHandle) const
	{
		return g_assetManager->IsValidAssetHandle(assetHandle);
	}

	StreamingInstanceMap::StreamingInstance& StreamingInstanceMap::Get(StreamingInstanceID id)
	{
		std::scoped_lock lock(m_mutex);
		VT_PROFILE_LOCK_MARK(m_mutex);

		VT_ENSURE(m_streamingInstances.contains(id));
		return *m_streamingInstances.at(id);
	}
	const StreamingInstanceMap::StreamingInstance& StreamingInstanceMap::Get(StreamingInstanceID id) const
	{
		std::scoped_lock lock(m_mutex);
		VT_PROFILE_LOCK_MARK(m_mutex);

		VT_ENSURE(m_streamingInstances.contains(id));
		return *m_streamingInstances.at(id);
	}

	bool StreamingInstanceMap::Contains(StreamingInstanceID id) const
	{
		std::scoped_lock lock(m_mutex);
		VT_PROFILE_LOCK_MARK(m_mutex);

		return m_streamingInstances.contains(id);
	}

	StreamingInstanceMap::StreamingInstance& StreamingInstanceMap::Add(StreamingInstanceID id)
	{
		std::scoped_lock lock(m_mutex);
		VT_PROFILE_LOCK_MARK(m_mutex);

		StreamingInstance* newInstance = m_instanceAllocator.Allocate();
		m_streamingInstances[id] = newInstance;

		return *newInstance;
	}

	void StreamingInstanceMap::Erase(StreamingInstanceID id)
	{
		std::scoped_lock lock(m_mutex);
		VT_PROFILE_LOCK_MARK(m_mutex);

		StreamingInstance* instance = m_streamingInstances.at(id);
		m_streamingInstances.erase(id);

		m_instanceAllocator.Free(instance);
	}
}
