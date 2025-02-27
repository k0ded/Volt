#pragma once

#include <glm/glm.hpp>

namespace Volt
{
	struct RayCastHit
	{
		PhysicsActorID actorId;
		glm::vec3 position;
		glm::vec3 normal;
		float distance;
	};
}
