#pragma once

#include "Volt-Physics/Config.h"

#include <EntitySystem/EntityHelper.h>

#include <PhysicsInterface/PhysicsTypes.h>

namespace Volt
{
	class EntityScene;
	class PhysicsScene;

	class VTP_API EntityPhysicsScene
	{
	public:
		EntityPhysicsScene(EntityScene& entityScene);
		~EntityPhysicsScene();

		void Update(float deltaTime);

	private:
		void ExecuteRigidbodySystem();
		void CreateActorFromEntity(EntityHelper entity);

		Ref<PhysicsScene> m_physicsScene;

		EntityScene& m_entityScene;
		vt::map<PhysicsActorID, EntityID> m_physicsActorToEntity;
		vt::map<EntityID, PhysicsActorID> m_entityToPhysicsActor;
		vt::map<EntityID, PhysicsActorID> m_entityToPhysicsControllerActor;

		UUID64 m_transformChangedCallbackID = 0;
		UUID64 m_entityDestroyedCallbackID = 0;

		std::atomic_bool m_isHandlingPhysicsUpdate = false;
	};
}
