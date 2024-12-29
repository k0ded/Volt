#include "vppch.h"
#include "Volt-Physics/RigidbodyComponent.h"

namespace Volt
{
	void RigidbodyComponent::OnCreate(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//auto sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//Physics::CreateActor(sceneEntity);
	}

	void RigidbodyComponent::OnDestroy(PhysicsEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//auto sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetActor(sceneEntity);
		//if (actor)
		//{
		//	Physics::GetScene()->RemoveActor(actor);
		//}
	}

	void RigidbodyComponent::OnTransformChanged(PhysicsTransformEntity entity)
	{
		//if (!SceneManager::IsPlaying())
		//{
		//	return;
		//}

		//auto sceneEntity = SceneManager::GetActiveScene()->GetSceneEntityFromScriptingEntity(entity);
		//auto actor = Physics::GetScene()->GetActor(sceneEntity);
		//if (actor)
		//{
		//	actor->SetPosition(entity.GetPosition(), true, false);
		//	actor->SetRotation(entity.GetRotation(), true, false);
		//}
	}
}
