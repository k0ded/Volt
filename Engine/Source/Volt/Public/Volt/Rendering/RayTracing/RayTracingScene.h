#pragma once

#include "Volt/Rendering/RayTracing/RayTracingInstance.h"

#include <EntitySystem/EntityID.h>

#include <RHIModule/RayTracing/AccelerationStructure.h>

#include <CoreUtilities/Containers/Vector.h>

namespace Volt
{
	class Scene;

	class RayTracingScene
	{
	public:
		RayTracingScene(Scene* scene);

		void Build();
		
		RayTracingInstanceID AddInstance(Ref<Mesh> mesh, EntityID entityId);
		void RemoveInstance(RayTracingInstanceID instanceId);

		VT_NODISCARD VT_INLINE RefPtr<RHI::AccelerationStructure> GetAccelerationStructure() const { return m_accelerationStructure; }

	private:
		RefPtr<RHI::AccelerationStructure> m_accelerationStructure;
		RefPtr<RHI::StorageBuffer> m_instancesBuffer;

		Vector<RayTracingInstance> m_instances;

		Scene* m_scene = nullptr;
	};
}
