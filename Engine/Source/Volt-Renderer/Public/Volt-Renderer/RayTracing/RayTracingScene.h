#pragma once

#include "Volt-Renderer/RayTracing/RayTracingInstance.h"

#include <RenderCore/Resources/GrowingGPUBuffer.h>

#include <EntitySystem/EntityID.h>

#include <RHIModule/RayTracing/AccelerationStructure.h>
#include <RHIModule/Synchronization/Fence.h>

#include <CoreUtilities/Containers/Vector.h>

namespace Volt
{
	class EntityScene;

	class RayTracingScene
	{
	public:
		RayTracingScene(EntityScene* scene);

		void Update();
		void WaitForCompletedUpdate() const;

		void RebuildAccelerationStructure();
		void UpdateAccelerationStructure();
		
		RayTracingInstanceID AddInstance(Ref<Mesh> mesh, EntityID entityId, uint32_t renderScenePrimitiveIndex);
		void AddInstanceWithID(Ref<Mesh> mesh, EntityID entityId, uint32_t renderScenePrimitiveIndex, RayTracingInstanceID id);
		void RemoveInstance(RayTracingInstanceID instanceId);
		void InvalidateInstance(RayTracingInstanceID instanceId);

		VT_NODISCARD VT_INLINE bool IsValid() const
		{
			return m_accelerationStructure != nullptr;
		}

		VT_NODISCARD VT_INLINE RefPtr<RHI::AccelerationStructure> GetAccelerationStructure() const 
		{ 
			return m_accelerationStructure; 
		}

	private:
		enum class OperationType : uint8_t
		{
			Update,
			Add,
			Remove
		};

		struct Operation
		{
			OperationType operationType;
			RayTracingInstanceID instanceId;
			size_t index;
		};

		RefPtr<RHI::AccelerationStructure> m_accelerationStructure;
		Ref<GrowingGPUBuffer> m_instancesBuffer;

		RefPtr<RHI::Fence> m_updateFence;
		RefPtr<RHI::Fence> m_buildFence;

		Vector<RayTracingInstance> m_instances;
		Vector<Operation> m_frameOperations;

		EntityScene* m_scene = nullptr;
	};
}
