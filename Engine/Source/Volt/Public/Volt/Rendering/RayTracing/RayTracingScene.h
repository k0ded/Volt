#pragma once

#include "Volt/Rendering/RayTracing/RayTracingInstance.h"

#include <RHIModule/RayTracing/AccelerationStructure.h>

#include <CoreUtilities/Containers/Vector.h>

namespace Volt
{
	class RayTracingScene
	{
	public:
		void Build();
		
		RayTracingInstanceID AddInstance(Ref<Mesh> mesh, const glm::mat4& transform);
		void RemoveInstance(RayTracingInstanceID instanceId);

		VT_NODISCARD VT_INLINE RefPtr<RHI::AccelerationStructure> GetAccelerationStructure() const { return m_accelerationStructure; }

	private:
		RefPtr<RHI::AccelerationStructure> m_accelerationStructure;
		RefPtr<RHI::StorageBuffer> m_instancesBuffer;

		Vector<RayTracingInstance> m_instances;
	};
}
