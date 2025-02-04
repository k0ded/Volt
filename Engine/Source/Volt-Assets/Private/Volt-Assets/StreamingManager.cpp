#include "vtassetspch.h"

#include "Volt-Assets/StreamingManager.h"
#include "Volt-Assets/MeshAsset.h"
#include "Volt-Assets/MaterialAsset.h"

#include <Volt-Renderer/RenderScene/ScenePrimitiveData.h>
#include <Volt-Renderer/Renderer.h>
#include <Volt-Renderer/Mesh/Mesh.h>

#include <Volt-Core/AssetTypes.h>

#include <AssetSystem/AssetManager.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(StreamingManager, Engine, 0);

	StreamingManager::StreamingManager()
		: m_meshReferenceCounter(AssetTypes::Mesh),
		m_materialReferenceCounter(AssetTypes::Material)
	{
		VT_ENSURE(!s_instance);

		s_instance = this;

		m_meshReferenceCounter.SetAssetUpdatedCallback([&](AssetHandle meshHandle, const std::unordered_set<StreamingInstanceID>& streamingInstances, AssetChangedState state)
		{
			if (state == AssetChangedState::Updated)
			{
				for (const auto& instanceId : streamingInstances)
				{
					const auto& instance = m_streamingInstances.at(instanceId);
					InitializeScenePrimitiveFromInstance(instance);
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

		StreamingInstance instance;
		instance.entityId = description.entityId;
		instance.meshHandle = description.meshHandle;
		instance.materialHandles = description.materialHandles;
		instance.primitiveData = description.primitiveData;

		m_streamingInstances[newId] = instance;

		for (const auto& materialHandle : description.materialHandles)
		{
			m_materialReferenceCounter.AddReference(materialHandle, newId);
		}

		m_meshReferenceCounter.AddReference(description.meshHandle, newId);

		InitializeScenePrimitiveFromInstance(m_streamingInstances.at(newId));

		return newId;
	}

	void StreamingManager::RemoveInstance(StreamingInstanceID instanceId)
	{
		const auto& instance = m_streamingInstances.at(instanceId);

		m_meshReferenceCounter.RemoveReference(instance.meshHandle, instanceId);

		for (const auto& materialHandle : instance.materialHandles)
		{
			m_meshReferenceCounter.RemoveReference(materialHandle, instanceId);
		}

		if (m_streamingInstances.contains(instanceId))
		{
			m_streamingInstances.erase(instanceId);
		}
	}

	void StreamingManager::InvalidateInstance(StreamingInstanceID instanceId, const StreamingInstanceDescription& description)
	{
		if (!m_streamingInstances.contains(instanceId))
		{
			return;
		}

		for (const auto& materialHandle : m_streamingInstances[instanceId].materialHandles)
		{
			m_materialReferenceCounter.RemoveReference(materialHandle, instanceId);
		}

		m_meshReferenceCounter.RemoveReference(m_streamingInstances[instanceId].meshHandle, instanceId);

		for (const auto& materialHandle : description.materialHandles)
		{
			m_materialReferenceCounter.AddReference(materialHandle, instanceId);
		}

		m_meshReferenceCounter.AddReference(description.meshHandle, instanceId);

		m_streamingInstances[instanceId].meshHandle = description.meshHandle;
		m_streamingInstances[instanceId].materialHandles = description.materialHandles;

		InitializeScenePrimitiveFromInstance(m_streamingInstances[instanceId]);
	}

	void StreamingManager::InitializeScenePrimitiveFromInstance(const StreamingInstance& instance)
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

	StreamingInstanceAssetReferenceCounter::StreamingInstanceAssetReferenceCounter(AssetType assetType)
		: m_assetType(assetType)
	{
		m_assetUpdatedCallback = AssetManager::RegisterAssetUpdatedCallback(assetType, [&](AssetHandle assetHandle, AssetChangedState state)
		{
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
		m_streamingInstancesFromAssetHandle[assetHandle].emplace(instanceId);
	}
	
	void StreamingInstanceAssetReferenceCounter::RemoveReference(AssetHandle assetHandle, StreamingInstanceID instanceId)
	{
		VT_ENSURE(m_streamingInstancesFromAssetHandle.contains(assetHandle));
		VT_ENSURE(m_streamingInstancesFromAssetHandle.at(assetHandle).contains(instanceId));
	
		m_streamingInstancesFromAssetHandle.at(assetHandle).erase(instanceId);
	}

	void StreamingInstanceAssetReferenceCounter::SetAssetUpdatedCallback(AssetUpdatedFunc callbackFunc)
	{
		m_callbackFunction = callbackFunc;
	}
}
