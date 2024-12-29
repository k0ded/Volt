#include "vppch.h"

#include "Volt-Physics/Components.h"

namespace Volt
{
	void CharacterControllerComponent::OnCreate(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//Physics::CreateControllerActor(sceneEntity);
	}

	void CharacterControllerComponent::OnDestroy(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetControllerActor(sceneEntity);
		//if (actor)
		//{
		//	Physics::GetScene()->RemoveControllerActor(actor);
		//}
	}

	void CharacterControllerComponent::OnTransformChanged(PhysicsTransformEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetControllerActor(sceneEntity);
		//if (actor)
		//{
		//	actor->SetFootPosition(entity.GetPosition());
		//}
	}

	void BoxColliderComponent::OnCreate(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//auto& component = entity.GetComponent<BoxColliderComponent>();
		//
		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetActor(sceneEntity);
		//
		//if (actor && !component.added)
		//{
		//	actor->AddCollider(component, sceneEntity);
		//}
	}

	void BoxColliderComponent::OnDestroy(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetActor(sceneEntity);
		//
		//if (actor)
		//{
		//	actor->RemoveCollider(ColliderType::Box);
		//}
	}

	void SphereColliderComponent::OnCreate(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//auto& component = entity.GetComponent<SphereColliderComponent>();
		//
		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetActor(sceneEntity);
		//
		//if (actor && !component.added)
		//{
		//	actor->AddCollider(component, sceneEntity);
		//}
	}

	void SphereColliderComponent::OnDestroy(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetActor(sceneEntity);
		//
		//if (actor)
		//{
		//	actor->RemoveCollider(ColliderType::Sphere);
		//}
	}

	void CapsuleColliderComponent::OnCreate(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//auto& component = entity.GetComponent<CapsuleColliderComponent>();
		//
		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetActor(sceneEntity);
		//
		//if (actor && !component.added)
		//{
		//	actor->AddCollider(component, sceneEntity);
		//}
	}

	void CapsuleColliderComponent::OnDestroy(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetActor(sceneEntity);
		//
		//if (actor)
		//{
		//	actor->RemoveCollider(ColliderType::Capsule);
		//}
	}

	void MeshColliderComponent::OnCreate(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//auto& component = entity.GetComponent<MeshColliderComponent>();
		//
		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetActor(sceneEntity);
		//
		//if (actor && !component.added)
		//{
		//	actor->AddCollider(component, sceneEntity);
		//}
	}

	void MeshColliderComponent::OnDestroy(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//auto& component = entity.GetComponent<MeshColliderComponent>();
		//
		//Entity sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetActor(sceneEntity);
		//
		//if (actor)
		//{
		//	if (component.isConvex)
		//	{
		//		actor->RemoveCollider(ColliderType::ConvexMesh);
		//	}
		//	else
		//	{
		//		actor->RemoveCollider(ColliderType::TriangleMesh);
		//	}
		//}
	}
}
