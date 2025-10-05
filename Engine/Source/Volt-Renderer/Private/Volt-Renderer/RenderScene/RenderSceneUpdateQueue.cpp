#include "vrpch.h"

#include "Volt-Renderer/RenderScene/RenderSceneUpdateQueue.h"

namespace Volt
{

	RenderSceneUpdateQueue::RenderSceneUpdateQueue()
	{
		constexpr size_t NumMaxUpdates = 8192;
		m_updateQueue.Allocate(NumMaxUpdates);
	}

	UUID64 RenderSceneUpdateQueue::AddPrimitiveInstance(EntityID entityId, Ref<MotionWeaver> motionWeaver, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Add;
		queuedUpdate.type = UpdateType::Primitive;

		queuedUpdate.primitiveInfo.entityId = entityId;
		queuedUpdate.primitiveInfo.motionWeaver = motionWeaver;
		queuedUpdate.primitiveInfo.mesh = mesh;
		queuedUpdate.primitiveInfo.material = material;
		queuedUpdate.primitiveInfo.subMeshIndex = subMeshIndex;

		m_updateQueue.Emplace(queuedUpdate);

		return queuedUpdate.primitiveInfo.id;
	}

	UUID64 RenderSceneUpdateQueue::AddLightInstance(EntityID entityId, const SceneLightDescription& description)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Add;
		queuedUpdate.type = UpdateType::Light;
		queuedUpdate.lightInfo.entityId = entityId;
		queuedUpdate.lightInfo.lightDescription = description;

		m_updateQueue.Emplace(queuedUpdate);

		return queuedUpdate.lightInfo.id;
	}

	void RenderSceneUpdateQueue::RemovePrimitiveInstance(UUID64 id)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Remove;
		queuedUpdate.type = UpdateType::Primitive;
		queuedUpdate.id = id;

		m_updateQueue.Emplace(queuedUpdate);
	}

	void RenderSceneUpdateQueue::RemoveLightInstance(UUID64 id)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Remove;
		queuedUpdate.type = UpdateType::Light;
		queuedUpdate.id = id;

		m_updateQueue.Emplace(queuedUpdate);
	}

	void RenderSceneUpdateQueue::InvalidatePrimitiveInstance(UUID64 renderObject)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Invalidate;
		queuedUpdate.type = UpdateType::Primitive;
		queuedUpdate.id = renderObject;

		m_updateQueue.Emplace(queuedUpdate);
	}

	void RenderSceneUpdateQueue::InvalidateLightInstance(UUID64 id)
	{
		QueuedUpdate queuedUpdate{};
		queuedUpdate.operation = UpdateOperation::Invalidate;
		queuedUpdate.type = UpdateType::Light;
		queuedUpdate.id = id;

		m_updateQueue.Emplace(queuedUpdate);
	}

	bool RenderSceneUpdateQueue::TryPop(QueuedUpdate& outQueuedUpdate)
	{
		return m_updateQueue.Pop(outQueuedUpdate);
	}
}
