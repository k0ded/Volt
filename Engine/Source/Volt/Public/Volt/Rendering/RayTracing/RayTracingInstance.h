#pragma once

#include <CoreUtilities/UUID.h>

#include <glm/glm.hpp>

namespace Volt
{
	using RayTracingInstanceID = UUID64;

	class Mesh;
	struct RayTracingInstance
	{
		RayTracingInstanceID id;

		Ref<Mesh> mesh;
		glm::mat4 transform;
	};
}
