#pragma once

#include <EntitySystem/EntityID.h>

#include <CoreUtilities/UUID.h>

namespace Volt
{
	using RayTracingInstanceID = UUID64;

	class Mesh;
	struct RayTracingInstance
	{
		RayTracingInstanceID id;
		uint32_t renderScenePrimitiveIndex;
		
		EntityID entityId;
		Ref<Mesh> mesh;
	};
}
