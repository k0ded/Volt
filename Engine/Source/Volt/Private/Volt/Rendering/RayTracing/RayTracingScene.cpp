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
		RHI::FenceCreateInfo fenceInfo{};
		fenceInfo.createSignaled = true;

		m_buildFence = RHI::Fence::Create(fenceInfo);
		m_updateFence = RHI::Fence::Create(fenceInfo);
	}

	void RayTracingScene::RebuildAccelerationStructure()
	{
		VT_PROFILE_FUNCTION();

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
		asCreateInfo.flags = RHI::AccelerationStructureBuildFlags::PreferFastTrace | RHI::AccelerationStructureBuildFlags::AllowUpdate;
	
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
		buildGeometryInfo.flags = RHI::AccelerationStructureBuildFlags::PreferFastTrace | RHI::AccelerationStructureBuildFlags::AllowUpdate;
		buildGeometryInfo.mode = RHI::AccelerationStructureBuildMode::Build;

		RHI::AccelerationStructureBuildRanges buildRanges{};
		auto& buildRange = buildRanges.AddRange();
		buildRange.primitiveCount = static_cast<uint32_t>(instances.size());
		buildRange.primitiveOffset = 0;
		buildRange.firstVertex = 0;
		buildRange.transformOffset = 0;

		commandBuffer->BuildAccelerationStructures({ buildGeometryInfo }, { buildRanges });

		commandBuffer->End();

		m_buildFence->WaitUntilSignaled();
		m_buildFence->Reset();

		commandBuffer->ExecuteWithFence(m_buildFence);

		// #TODO_Ivar: Remove when design is finalized
		m_buildFence->WaitUntilSignaled();
	}

	void RayTracingScene::UpdateAccelerationStructure()
	{
		VT_PROFILE_FUNCTION();

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

		// Copy data to buffer
		{
			RHI::AccelerationStructureInstance* mappedInstances = m_instancesBuffer->Map<RHI::AccelerationStructureInstance>();
			memcpy(mappedInstances, instances.data(), sizeof(RHI::AccelerationStructureInstance) * instances.size());
			m_instancesBuffer->Unmap();
		}

		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		commandBuffer->Begin();

		RHI::AccelerationStructureBuildGeometryInfo buildGeometryInfo{};
		buildGeometryInfo.srcAccelerationStructure = m_accelerationStructure;
		buildGeometryInfo.dstAccelerationStructure = m_accelerationStructure;
		buildGeometryInfo.type = RHI::AccelerationStructureType::TopLevel;
		buildGeometryInfo.flags = RHI::AccelerationStructureBuildFlags::PreferFastTrace | RHI::AccelerationStructureBuildFlags::AllowUpdate;
		buildGeometryInfo.mode = RHI::AccelerationStructureBuildMode::Update;

		auto& instancesGeometry = buildGeometryInfo.geometries.emplace_back();
		instancesGeometry.geometryType = RHI::AccelerationStructureGeometryType::Instances;
		instancesGeometry.flags = RHI::AccelerationStructureGeometryFlags::Opaque;
		instancesGeometry.instancesBuffer = m_instancesBuffer;

		RHI::AccelerationStructureBuildRanges buildRanges{};
		auto& buildRange = buildRanges.AddRange();
		buildRange.primitiveCount = static_cast<uint32_t>(instances.size());
		buildRange.primitiveOffset = 0;
		buildRange.firstVertex = 0;
		buildRange.transformOffset = 0;

		commandBuffer->BuildAccelerationStructures({ buildGeometryInfo }, { buildRanges });

		commandBuffer->End();
		
		m_updateFence->WaitUntilSignaled();
		m_updateFence->Reset();
		
		commandBuffer->ExecuteWithFence(m_updateFence);

		m_updateFence->WaitUntilSignaled();
	}

	void RayTracingScene::Update()
	{
		VT_PROFILE_FUNCTION();

		bool shouldPerformFullRebuild = false;

		for (const auto& operation : m_frameOperations)
		{
			if (operation.operationType == OperationType::Add || operation.operationType == OperationType::Remove)
			{
				shouldPerformFullRebuild = true;
				break;
			}
		}

		shouldPerformFullRebuild |= m_instancesBuffer == nullptr;

		if (shouldPerformFullRebuild)
		{
			RebuildAccelerationStructure();
		}
		else
		{
			UpdateAccelerationStructure();
		}

		m_frameOperations.clear();
	}

	void RayTracingScene::WaitForCompletedUpdate() const
	{
		m_updateFence->WaitUntilSignaled();
		m_buildFence->WaitUntilSignaled();
	}
	
	RayTracingInstanceID RayTracingScene::AddInstance(Ref<Mesh> mesh, EntityID entityId)
	{
		auto& instance = m_instances.emplace_back();

		instance.entityId = entityId;
		instance.mesh = mesh;
		instance.id = {};

		auto& newOperation = m_frameOperations.emplace_back();
		newOperation.index = m_instances.size() - 1;
		newOperation.instanceId = instance.id;
		newOperation.operationType = OperationType::Add;

		return instance.id;
	}

	void RayTracingScene::RemoveInstance(RayTracingInstanceID instanceId)
	{
		auto it = std::find_if(m_instances.begin(), m_instances.end(), [instanceId](const RayTracingInstance& value)
		{
			return instanceId == value.id;
		});

		auto& newOperation = m_frameOperations.emplace_back();
		newOperation.index = std::distance(m_instances.begin(), it);
		newOperation.instanceId = instanceId;
		newOperation.operationType = OperationType::Remove;

		if (it != m_instances.end())
		{
			m_instances.erase(it);
		}
	}

	void RayTracingScene::InvalidateInstance(RayTracingInstanceID instanceId)
	{
		auto it = std::find_if(m_instances.begin(), m_instances.end(), [instanceId](const RayTracingInstance& value)
		{
			return instanceId == value.id;
		});

		if (it == m_instances.end())
		{
			return;
		}

		auto& newOperation = m_frameOperations.emplace_back();
		newOperation.index = std::distance(m_instances.begin(), it);
		newOperation.instanceId = instanceId;
		newOperation.operationType = OperationType::Update;
	}
}
