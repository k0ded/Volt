#pragma once

#include "Volt-Physics/Config.h"

#include "EntitySystem/EntityID.h"

#include <PhysicsInterface/PhysicsTypes.h>

namespace Volt
{
	class EntityScene;
	class PhysicsScene;

	class VTP_API EntityPhysicsScene
	{
	public:
		EntityPhysicsScene(EntityScene& entityScene);

		void Update(float deltaTime);

	private:
		Ref<PhysicsScene> m_physicsScene;

		EntityScene& m_entityScene;
		vt::map<PhysicsActorID, EntityID> m_physicsActorToEntity;
	};
}
