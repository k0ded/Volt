#include "vtccpch.h"

#include "Volt-CoreComponents/RenderingComponents.h"

#include <Volt-Assets/StreamingManager.h>

#include <Volt-Renderer/RenderScene/ScenePrimitiveData.h>
#include <Volt-Renderer/RenderScene.h>
#include <Volt-Renderer/Camera/Camera.h>
#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Material.h>

#include <Volt-Animation/MotionWeaver.h>

#include <AssetSystem/AssetManager.h>

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

	void MeshComponent::OnCreate(MeshEntity entity)
	{
		auto& meshComponent = entity.GetComponent<MeshComponent>();
		meshComponent.m_scenePrimitiveData = CreateRef<ScenePrimitiveData>(entity.GetID(), entity.GetRenderScene());
		meshComponent.m_streamingInstanceID = StreamingManager::Get().AddInstance(CreateStreamingInstanceDescription(meshComponent, entity.GetID(), meshComponent.m_scenePrimitiveData));
	}

	void MeshComponent::OnDestroy(MeshEntity entity)
	{
		auto& component = entity.GetComponent<MeshComponent>();

		StreamingManager::Get().RemoveInstance(component.m_streamingInstanceID);
		component.m_scenePrimitiveData = nullptr;
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

	void MeshComponent::OnComponentCopied(MeshEntity entity)
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

	void CameraComponent::OnCreate(CameraEntity entity)
	{
		auto& component = entity.GetComponent<CameraComponent>();
		component.camera = CreateRef<Camera>(component.fieldOfView, 1.f, 16.f / 9.f, component.nearPlane, component.farPlane);
	}

	void MotionWeaveComponent::OnStart(WeaveEntity entity)
	{
		//const auto& meshComponent = entity.GetComponent<MeshComponent>();
		//auto& weaveComponent = entity.GetComponent<MotionWeaveComponent>();

		//auto scene = SceneManager::GetActiveScene();

		//auto sceneEntity = scene->GetSceneEntityFromScriptingEntity(entity);

		//weaveComponent.MotionWeaver = MotionWeaver::Create(weaveComponent.motionWeaveDatabase);

		//Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshComponent.handle);
		//if (mesh && mesh->IsValid())
		//{
		//	const auto& materialTable = mesh->GetMaterialTable();

		//	for (size_t i = 0; i < mesh->GetSubMeshes().size(); i++)
		//	{
		//		auto material = AssetManager::QueueAsset<Material>(materialTable.GetMaterial(mesh->GetSubMeshes().at(i).materialIndex));
		//		if (!material->IsValid())
		//		{
		//		}

		//		auto uuid = scene->GetRenderScene()->AddInstance(entity.GetID(), weaveComponent.MotionWeaver, mesh, material, static_cast<uint32_t>(i));
		//		weaveComponent.renderObjectIds.emplace_back(uuid);
		//	}
		//}
	}
}
