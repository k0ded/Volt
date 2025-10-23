#pragma once

#include "Volt-Renderer/RenderScene/SceneLightData.h"

#include <CoreUtilities/UUID.h>
#include <EntitySystem/EntityID.h>

namespace Volt
{
	class Mesh;
	class RenderMaterial;

	using RenderPrimitiveID = UUID64;

	struct RenderPrimitiveData
	{
		RenderPrimitiveID id;
		EntityID entityId;
	
		Weak<Mesh> mesh;
		Weak<RenderMaterial> material;

		uint32_t subMeshIndex = 0;
		uint32_t vertexBufferIndex = 0;
		uint32_t meshletStartOffset = 0;
		uint32_t primitiveIndex = 0;
	};

	inline bool operator==(const RenderPrimitiveData& lhs, const RenderPrimitiveData& rhs) { return lhs.id == rhs.id; }
	inline bool operator!=(const RenderPrimitiveData& lhs, const RenderPrimitiveData& rhs) { return !(lhs == rhs); }

	inline bool operator==(const RenderPrimitiveData& lhs, const RenderPrimitiveID& rhs) { return lhs.id == rhs; }
	inline bool operator!=(const RenderPrimitiveData& lhs, const RenderPrimitiveID& rhs) { return !(lhs == rhs); }

	struct RenderLightData
	{
		RenderPrimitiveID id;
		EntityID entityId;

		SceneLightDescription description;
	};

	inline bool operator==(const RenderLightData& lhs, const RenderLightData& rhs) { return lhs.id == rhs.id; }
	inline bool operator!=(const RenderLightData& lhs, const RenderLightData& rhs) { return !(lhs == rhs); }

	inline bool operator==(const RenderLightData& lhs, const RenderPrimitiveID& rhs) { return lhs.id == rhs; }
	inline bool operator!=(const RenderLightData& lhs, const RenderPrimitiveID& rhs) { return !(lhs == rhs); }
}
