#pragma once

#include "Volt-Renderer/RenderScene/SceneLightData.h"
#include "Volt-Renderer/RayTracing/RayTracingInstance.h"
#include "Volt-Renderer/RenderPrimitiveData.h"

#include <EntitySystem/EntityID.h>

#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Variant.h>

namespace Volt
{
	class TempAnimator;
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
			Light,
			RayTracingInstance
		};

		struct PrimitiveAddInfo
		{
			EntityID entityId;
			Ref<TempAnimator> animator;
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

		struct RayTracingAddInfo
		{
			Ref<Mesh> mesh;
			EntityID entityId;
			RenderPrimitiveID renderScenePrimitiveId;

			RayTracingInstanceID id;
		};

		struct QueuedUpdate
		{
			UpdateOperation operation;
			UpdateType type;

			// ID is used for all remove and invalidation operations.
			PrimitiveAddInfo primitiveInfo;
			LightAddInfo lightInfo;
			RayTracingAddInfo rayTracingInstanceInfo;
			UUID64 id = 0;
		};

		RenderSceneUpdateQueue();

		UUID64 AddPrimitiveInstance(EntityID entityId, Ref<TempAnimator> animator, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex);
		UUID64 AddLightInstance(EntityID entityId, const SceneLightDescription& description);
		UUID64 AddRayTracingInstance(EntityID entityId, Ref<Mesh> mesh, RenderPrimitiveID primitiveId);

		void RemovePrimitiveInstance(UUID64 id);
		void RemoveLightInstance(UUID64 id);
		void RemoveRayTracingInstance(RayTracingInstanceID id);

		void InvalidatePrimitiveInstance(UUID64 renderObject);
		void InvalidateLightInstance(UUID64 id);
		void InvalidateRayTracingInstance(RayTracingInstanceID id);

		bool TryPop(QueuedUpdate& outQueuedUpdate);

	private:
		WorkQueue<QueuedUpdate, QueueThreadingPolicy::MPSC> m_updateQueue;
	};
}
