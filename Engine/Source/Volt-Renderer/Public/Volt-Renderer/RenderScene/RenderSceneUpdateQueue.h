#pragma once

#include "Volt-Renderer/RenderScene/SceneLightData.h"

#include <EntitySystem/EntityID.h>

#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Variant.h>

namespace Volt
{
	class Mesh;
	class RenderMaterial;

	class RenderSceneUpdateQueue
	{
	public:
		enum class UpdateOperation : uint8_t
		{
			Add,
			Remove,
			Invalidate
		};

		enum class UpdateType : uint8_t
		{
			Primitive,
			Light
		};

		struct PrimitiveAddInfo
		{
			EntityID entityId;
			Ref<Mesh> mesh;
			Ref<RenderMaterial> material;
			uint32_t subMeshIndex;

			UUID64 id;
		};

		struct LightAddInfo
		{
			EntityID entityId;
			SceneLightDescription lightDescription;

			UUID64 id;
		};

		struct QueuedUpdate
		{
			UpdateOperation operation;
			UpdateType type;

			// ID is used for all remove and invalidation operations.
			PrimitiveAddInfo primitiveInfo;
			LightAddInfo lightInfo;
			UUID64 id;
		};

		RenderSceneUpdateQueue();

		UUID64 AddPrimitiveInstance(EntityID entityId, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex);
		UUID64 AddLightInstance(EntityID entityId, const SceneLightDescription& description);

		void RemovePrimitiveInstance(UUID64 id);
		void RemoveLightInstance(UUID64 id);

		void InvalidatePrimitiveInstance(UUID64 renderObject);
		void InvalidateLightInstance(UUID64 id);

		bool TryPop(QueuedUpdate& outQueuedUpdate);

	private:
		WorkQueue<QueuedUpdate, QueueThreadingPolicy::MPSC> m_updateQueue;
	};
}
