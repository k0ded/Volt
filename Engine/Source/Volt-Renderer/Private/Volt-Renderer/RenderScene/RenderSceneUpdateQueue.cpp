#include "vrpch.h"

#include "Volt-Renderer/RenderScene/RenderSceneUpdateQueue.h"

namespace Volt
{

	RenderSceneUpdateQueue::RenderSceneUpdateQueue()
	{
		constexpr size_t NumMaxUpdates = 65536;
		m_updateQueue.Allocate(NumMaxUpdates);
	}

	UUID64 RenderSceneUpdateQueue::AddPrimitiveInstance(EntityID entityId, Ref<TempAnimator> animator, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Add;
		queuedUpdate.type = UpdateType::Primitive;

		queuedUpdate.primitiveInfo.entityId = entityId;
		queuedUpdate.primitiveInfo.animator = animator;
		queuedUpdate.primitiveInfo.mesh = mesh;
		queuedUpdate.primitiveInfo.material = material;
		queuedUpdate.primitiveInfo.subMeshIndex = subMeshIndex;

		VT_MAYBE_UNUSED bool wasEmplaced = m_updateQueue.Emplace(queuedUpdate);
		VT_ENSURE(wasEmplaced);

		return queuedUpdate.primitiveInfo.id;
	}

	UUID64 RenderSceneUpdateQueue::AddLightInstance(EntityID entityId, const SceneLightDescription& description)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Add;
		queuedUpdate.type = UpdateType::Light;
		queuedUpdate.lightInfo.entityId = entityId;
		queuedUpdate.lightInfo.lightDescription = description;

		VT_MAYBE_UNUSED bool wasEmplaced = m_updateQueue.Emplace(queuedUpdate);
		VT_ENSURE(wasEmplaced);

		return queuedUpdate.lightInfo.id;
	}

	UUID64 RenderSceneUpdateQueue::AddRayTracingInstance(EntityID entityId, Ref<Mesh> mesh, RenderPrimitiveID primitiveId)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Add;
		queuedUpdate.type = UpdateType::RayTracingInstance;
		queuedUpdate.rayTracingInstanceInfo.entityId = entityId;
		queuedUpdate.rayTracingInstanceInfo.mesh = mesh;
		queuedUpdate.rayTracingInstanceInfo.renderScenePrimitiveId = primitiveId;

		VT_MAYBE_UNUSED bool wasEmplaced = m_updateQueue.Emplace(queuedUpdate);
		VT_ENSURE(wasEmplaced);

		return queuedUpdate.rayTracingInstanceInfo.id;
	}

	void RenderSceneUpdateQueue::RemovePrimitiveInstance(UUID64 id)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Remove;
		queuedUpdate.type = UpdateType::Primitive;
		queuedUpdate.id = id;

		VT_MAYBE_UNUSED bool wasEmplaced = m_updateQueue.Emplace(queuedUpdate);
		VT_ENSURE(wasEmplaced);
	}

	void RenderSceneUpdateQueue::RemoveLightInstance(UUID64 id)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Remove;
		queuedUpdate.type = UpdateType::Light;
		queuedUpdate.id = id;

		VT_MAYBE_UNUSED bool wasEmplaced = m_updateQueue.Emplace(queuedUpdate);
		VT_ENSURE(wasEmplaced);
	}

	void RenderSceneUpdateQueue::RemoveRayTracingInstance(RayTracingInstanceID id)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Remove;
		queuedUpdate.type = UpdateType::RayTracingInstance;
		queuedUpdate.id = id;

		VT_MAYBE_UNUSED bool wasEmplaced = m_updateQueue.Emplace(queuedUpdate);
		VT_ENSURE(wasEmplaced);
	}

	void RenderSceneUpdateQueue::InvalidatePrimitiveInstance(UUID64 renderObject)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Invalidate;
		queuedUpdate.type = UpdateType::Primitive;
		queuedUpdate.id = renderObject;

		VT_MAYBE_UNUSED bool wasEmplaced = m_updateQueue.Emplace(queuedUpdate);
		VT_ENSURE(wasEmplaced);
	}

	void RenderSceneUpdateQueue::InvalidateLightInstance(UUID64 id)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Invalidate;
		queuedUpdate.type = UpdateType::Light;
		queuedUpdate.id = id;

		VT_MAYBE_UNUSED bool wasEmplaced = m_updateQueue.Emplace(queuedUpdate);
		VT_ENSURE(wasEmplaced);
	}

	void RenderSceneUpdateQueue::InvalidateRayTracingInstance(RayTracingInstanceID id)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Invalidate;
		queuedUpdate.type = UpdateType::RayTracingInstance;
		queuedUpdate.id = id;

		VT_MAYBE_UNUSED bool wasEmplaced = m_updateQueue.Emplace(queuedUpdate);
		VT_ENSURE(wasEmplaced);
	}

	bool RenderSceneUpdateQueue::TryPop(QueuedUpdate& outQueuedUpdate)
	{
		return m_updateQueue.Pop(outQueuedUpdate);
	}
}
