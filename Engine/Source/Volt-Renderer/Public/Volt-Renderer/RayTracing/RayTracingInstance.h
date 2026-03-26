#pragma once

#include <EntitySystem/EntityID.h>

#include <CoreUtilities/UUID.h>
#include <CoreUtilities/Pointers/Ref.h>

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
