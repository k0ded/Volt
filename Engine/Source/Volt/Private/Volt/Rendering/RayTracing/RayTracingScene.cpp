#include "vtpch.h"

#include "Volt/Rendering/RayTracing/RayTracingScene.h"
#include "Volt/Rendering/RayTracing/RayTracingSceneGeometry.h"
#include "Volt/Asset/Mesh/Mesh.h"
#include "Volt/Scene/Scene.h"
#include "Volt/Scene/Entity.h"

#include <RHIModule/Buffers/CommandBuffer.h>

namespace Volt
{
	RayTracingScene::RayTracingScene(Scene* scene)
		: m_scene(scene)
	{
	}

	void RayTracingScene::Build()
	{
		Vector<RHI::AccelerationStructureInstance> instances;
		instances.reserve(m_instances.size());

		for (const auto& instance : m_instances)
		{
			auto entity = m_scene->GetEntityFromID(instance.entityId);
			if (!entity)
			{
				continue;
			}

			auto& rtInstance = instances.emplace_back();
			rtInstance.transform = glm::transpose(entity.GetTransform());
			rtInstance.instanceCustomIndex = 0;
			rtInstance.mask = 0xFF;
			rtInstance.instanceShaderBindingTableRecordOffset = 0;
			rtInstance.flags = (uint32_t)RHI::AccelerationStructureGeometryInstanceFlags::None;
			rtInstance.accelerationStructureReference = instance.mesh->GetRayTracingSceneGeometry()->GetAccelerationStructureDeviceAddress();
		}

		m_instancesBuffer = RHI::StorageBuffer::Create(static_cast<uint32_t>(instances.size()), sizeof(RHI::AccelerationStructureInstance), "Ray Tracing Scene TLAS", RHI::BufferUsage::DeviceAddress | RHI::BufferUsage::AccelerationStructureInput, RHI::MemoryUsage::CPUToGPU);
	
		// Copy data to buffer
		{
			RHI::AccelerationStructureInstance* mappedInstances = m_instancesBuffer->Map<RHI::AccelerationStructureInstance>();
			memcpy(mappedInstances, instances.data(), sizeof(RHI::AccelerationStructureInstance) * instances.size());
			m_instancesBuffer->Unmap();
		}

		RHI::AccelerationStructureCreateInfo asCreateInfo;
		asCreateInfo.type = RHI::AccelerationStructureType::TopLevel;
		asCreateInfo.flags = RHI::AccelerationStructureBuildFlags::PreferFastTrace;
	
		auto& asInstances = asCreateInfo.geometries.emplace_back();
		asInstances.geometryType = RHI::AccelerationStructureGeometryType::Instances;
		asInstances.flags = RHI::AccelerationStructureGeometryFlags::Opaque;
		asInstances.instancesBuffer = m_instancesBuffer;

		m_accelerationStructure = RHI::AccelerationStructure::Create(asCreateInfo);

		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		commandBuffer->Begin();

		RHI::AccelerationStructureBuildGeometryInfo buildGeometryInfo{};
		buildGeometryInfo.geometries = asCreateInfo.geometries;
		buildGeometryInfo.srcAccelerationStructure = nullptr;
		buildGeometryInfo.dstAccelerationStructure = m_accelerationStructure;
		buildGeometryInfo.type = RHI::AccelerationStructureType::TopLevel;
		buildGeometryInfo.flags = RHI::AccelerationStructureBuildFlags::PreferFastTrace;
		buildGeometryInfo.mode = RHI::AccelerationStructureBuildMode::Build;

		RHI::AccelerationStructureBuildRanges buildRanges{};
		auto& buildRange = buildRanges.AddRange();
		buildRange.primitiveCount = static_cast<uint32_t>(instances.size());
		buildRange.primitiveOffset = 0;
		buildRange.firstVertex = 0;
		buildRange.transformOffset = 0;

		commandBuffer->BuildAccelerationStructures({ buildGeometryInfo }, { buildRanges });

		commandBuffer->End();
		commandBuffer->ExecuteAndWait();
	}
	
	RayTracingInstanceID RayTracingScene::AddInstance(Ref<Mesh> mesh, EntityID entityId)
	{
		auto& instance = m_instances.emplace_back();

		instance.entityId = entityId;
		instance.mesh = mesh;
		instance.id = {};

		Build();

		return instance.id;
	}

	void RayTracingScene::RemoveInstance(RayTracingInstanceID instanceId)
	{
		auto it = std::find_if(m_instances.begin(), m_instances.end(), [instanceId](const RayTracingInstance& value)
		{
			return instanceId == value.id;
		});

		if (it != m_instances.end())
		{
			m_instances.erase(it);
		}
	}
}
