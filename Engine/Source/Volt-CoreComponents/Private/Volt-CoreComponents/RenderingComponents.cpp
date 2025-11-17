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

	void MeshComponent::OnMemberChanged(MeshEntity entity)
	{
		auto& component = entity.GetComponent<MeshComponent>();

		if (component.handle == Asset::Null())
		{
			return;
		}

		StreamingManager::Get().InvalidateInstance(component.m_streamingInstanceID, CreateStreamingInstanceDescription(component, entity.GetID(), component.m_scenePrimitiveData));
	}

	void MeshComponent::OnTransformChanged(MeshEntity entity)
	{
		auto& meshComponent = entity.GetComponent<MeshComponent>();
		meshComponent.m_scenePrimitiveData->Invalidate();
	}

	void CameraComponent::OnInitialize(CameraEntity entity)
	{
		auto& component = entity.GetComponent<CameraComponent>();
		component.camera = CreateRef<Camera>(glm::radians(component.fieldOfView), 1.f, 16.f / 9.f, component.nearPlane, component.farPlane);
	}
}
