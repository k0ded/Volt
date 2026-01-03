#include "vtccpch.h"

#include "Volt-CoreComponents/RenderingComponents.h"

#include <Volt-Assets/StreamingManager.h>

#include <Volt-Renderer/RenderScene/ScenePrimitiveData.h>
#include <Volt-Renderer/RenderScene.h>
#include <Volt-Renderer/Camera/Camera.h>
#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Animation/TempAnimator.h>

namespace Volt
{
	VT_REGISTER_COMPONENT(MeshComponent);
	VT_REGISTER_COMPONENT(CameraComponent);
	VT_REGISTER_COMPONENT(TextRendererComponent);
	VT_REGISTER_COMPONENT(SpriteComponent);
	VT_REGISTER_COMPONENT(DecalComponent);

	StreamingInstanceDescription CreateStreamingInstanceDescription(const MeshComponent& meshComponent, EntityID entityId, Ref<ScenePrimitiveData> scenePrimitiveData)
	{
		StreamingInstanceDescription streamingInstanceDescription;
		streamingInstanceDescription.entityId = entityId;
		streamingInstanceDescription.meshHandle = meshComponent.handle;
		streamingInstanceDescription.materialHandles = meshComponent.materials;
		streamingInstanceDescription.primitiveData = scenePrimitiveData;
		return streamingInstanceDescription;
	}

	void MeshComponent::OnDestroy(MeshEntity entity)
	{
		auto& component = entity.GetComponent<MeshComponent>();

		if (StreamingManager::IsValid())
		{
			StreamingManager::Get().RemoveInstance(component.m_streamingInstanceID);
		}
		component.m_scenePrimitiveData = nullptr;
	}

	void MeshComponent::OnIntitialize(MeshEntity entity)
	{
		auto& meshComponent = entity.GetComponent<MeshComponent>();

		meshComponent.m_prevHandle = meshComponent.handle;

		Ref<TempAnimator> animator;
		if (entity.HasComponent<AnimationPlayerComponent>())
		{
			AnimationPlayerComponent& animatorComponent = entity.GetComponent<AnimationPlayerComponent>();
			animator = animatorComponent.animator = CreateRef<TempAnimator>(animatorComponent.skeletonHandle, animatorComponent.animationHandle);
		}

		if (animator)
		{
			meshComponent.m_scenePrimitiveData = CreateRef<ScenePrimitiveData>(entity.GetID(), entity.GetRenderScene(), animator);
		}
		else
		{
			meshComponent.m_scenePrimitiveData = CreateRef<ScenePrimitiveData>(entity.GetID(), entity.GetRenderScene());
		}
		meshComponent.m_streamingInstanceID = StreamingManager::Get().AddInstance(CreateStreamingInstanceDescription(meshComponent, entity.GetID(), meshComponent.m_scenePrimitiveData));
	}

	void MeshComponent::SetMesh(AssetHandle meshHandle, EntityID owningEntityId)
	{
		if (meshHandle == handle)
		{
			return;
		}

		handle = meshHandle;
		m_prevHandle = handle;

		UpdateMesh();
		StreamingManager::Get().InvalidateInstance(m_streamingInstanceID, CreateStreamingInstanceDescription(*this, owningEntityId, m_scenePrimitiveData));
	}

	void MeshComponent::OnMemberChanged(MeshEntity entity)
	{
		auto& component = entity.GetComponent<MeshComponent>();

		if (component.handle == Asset::Null())
		{
			component.materials.clear();
			return;
		}

		if (component.handle != component.m_prevHandle)
		{
			component.m_prevHandle = component.handle;
			component.UpdateMesh();
		}

		StreamingManager::Get().InvalidateInstance(component.m_streamingInstanceID, CreateStreamingInstanceDescription(component, entity.GetID(), component.m_scenePrimitiveData));
	}

	void MeshComponent::OnTransformChanged(MeshEntity entity)
	{
		auto& meshComponent = entity.GetComponent<MeshComponent>();
		meshComponent.m_scenePrimitiveData->Invalidate();
	}

	void MeshComponent::UpdateMesh()
	{
		// When changing mesh we need to setup the correct materials for it.
		materials.clear();

		ReadOnlyAssetMetadata meshMetadata = g_assetManager->GetReadOnlyAssetMetadata(handle);
		if (meshMetadata.IsValid())
		{
			const MeshCustomMetadata& customMeshMetadata = meshMetadata->GetCustomData<MeshCustomMetadata>();
			materials = customMeshMetadata.materialReferences;
		}
	}

	void CameraComponent::OnInitialize(CameraEntity entity)
	{
		auto& component = entity.GetComponent<CameraComponent>();
		component.camera = CreateRef<Camera>(glm::radians(component.fieldOfView), 1.f, 16.f / 9.f, component.nearPlane, component.farPlane);
	}
}
