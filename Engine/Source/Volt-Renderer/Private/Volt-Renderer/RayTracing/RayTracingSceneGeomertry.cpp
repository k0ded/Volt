#include "vrpch.h"

#include "Volt-Renderer/RayTracing/RayTracingSceneGeometry.h"

#include <RenderCore/CommandBufferPool.h>

#include <RHIModule/Buffers/Buffer.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>

namespace Volt
{
	RayTracingSceneGeometry::RayTracingSceneGeometry(const RayTracingSceneGeometryCreateInfo& createInfo)
	{
		RHI::AccelerationStructureCreateInfo asCreateInfo;
		asCreateInfo.type = RHI::AccelerationStructureType::BottomLevel;

		for (const auto& geom : createInfo.geometries)
		{
			auto& asGeometry = asCreateInfo.geometries.emplace_back();
			asGeometry.geometryType = RHI::AccelerationStructureGeometryType::Triangles;
			asGeometry.vertexFormat = RHI::PixelFormat::R32G32B32_SFLOAT;
			asGeometry.vertexStride = static_cast<uint32_t>(createInfo.vertexPositionsBuffer->GetElementSize());
			asGeometry.vertexCount = geom.vertexCount;
			asGeometry.indexCount = geom.indexCount;
			asGeometry.indexType = RHI::IndexType::UInt32;
			asGeometry.flags = RHI::AccelerationStructureGeometryFlags::Opaque;
			asGeometry.vertexPositionsBuffer = createInfo.vertexPositionsBuffer;
			asGeometry.indexBuffer = createInfo.indexBuffer;
		}

		asCreateInfo.flags = RHI::AccelerationStructureBuildFlags::PreferFastTrace;
		asCreateInfo.type = RHI::AccelerationStructureType::BottomLevel;

		m_accelerationStructure = RHI::AccelerationStructure::Create(asCreateInfo);

		RefPtr<PooledCommandBuffer> pooledCommandBuffer = CommandBufferPool::GetCommandBuffer();
		RefPtr<RHI::CommandBuffer> commandBuffer = pooledCommandBuffer->Get();

		commandBuffer->Begin();

		RHI::AccelerationStructureBuildGeometryInfo buildGeometryInfo{};
		buildGeometryInfo.geometries = asCreateInfo.geometries;
		buildGeometryInfo.srcAccelerationStructure = nullptr;
		buildGeometryInfo.dstAccelerationStructure = m_accelerationStructure;
		buildGeometryInfo.type = RHI::AccelerationStructureType::BottomLevel;
		buildGeometryInfo.flags = RHI::AccelerationStructureBuildFlags::PreferFastTrace;
		buildGeometryInfo.mode = RHI::AccelerationStructureBuildMode::Build;

		RHI::AccelerationStructureBuildRanges buildRanges{};

		for (const auto& geom : createInfo.geometries)
		{
			auto& newRange = buildRanges.AddRange();
			newRange.firstVertex = geom.vertexOffset;
			newRange.primitiveCount = geom.indexCount / 3;
			newRange.primitiveOffset = geom.indexOffset / 3;
		}

		commandBuffer->BuildAccelerationStructures({ buildGeometryInfo }, { buildRanges });

		commandBuffer->End();
		RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
	}

	uint64_t RayTracingSceneGeometry::GetAccelerationStructureDeviceAddress() const
	{
		return m_accelerationStructure->GetDeviceAddress();
	}

	Ref<RayTracingSceneGeometry> RayTracingSceneGeometry::Create(const RayTracingSceneGeometryCreateInfo& createInfo)
	{
		return CreateRef<RayTracingSceneGeometry>(createInfo);
	}
}
