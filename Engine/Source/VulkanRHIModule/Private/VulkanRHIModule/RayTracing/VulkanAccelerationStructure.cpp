#include "vkpch.h"

#include "VulkanRHIModule/RayTracing/VulkanAccelerationStructure.h"
#include "VulkanRHIModule/RayTracing/VulkanRayTracingHelpers.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsDevice.h"

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Graphics/GraphicsContext.h>

#include <RHIModule/RHIProxy.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanAccelerationStructure::VulkanAccelerationStructure(const AccelerationStructureCreateInfo& createInfo)
	{
		InitializeFromInfo(createInfo);
	}

	VulkanAccelerationStructure::~VulkanAccelerationStructure()
	{
		if (!m_handle)
		{
			return;
		}

		RHIProxy::GetInstance().DestroyResource([handle = m_handle]() 
		{
			GraphicsContext::GetDevice()->As<VulkanGraphicsDevice>()->WaitForIdle(); // #TODO_Ivar: Should not be called.
			vkDestroyAccelerationStructureKHR(GraphicsContext::GetDevice()->GetHandle<VkDevice>(), handle, nullptr);
		});
	}

	uint64_t VulkanAccelerationStructure::GetDeviceAddress() const
	{
		return m_deviceAddress;
	}

	void* VulkanAccelerationStructure::GetHandleImpl() const
	{
		return m_handle;
	}

	void VulkanAccelerationStructure::InitializeFromInfo(const AccelerationStructureCreateInfo& createInfo)
	{
		Vector<VkAccelerationStructureGeometryKHR> geometries;
		VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;

		Vector<uint32_t> primitiveCounts;

		for (const auto& geometryInfo : createInfo.geometries)
		{
			auto& vulkanGeometry = geometries.emplace_back();
			vulkanGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			vulkanGeometry.pNext = nullptr;
			vulkanGeometry.geometryType = Utility::GetGeometryType(geometryInfo.geometryType);
			vulkanGeometry.flags = Utility::GetGeometryFlags(geometryInfo.flags);

			if (geometryInfo.geometryType == AccelerationStructureGeometryType::Triangles)
			{
				VT_ENSURE(geometryInfo.indexBuffer && geometryInfo.vertexPositionsBuffer);

				auto& triangles = vulkanGeometry.geometry.triangles;
				triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
				triangles.pNext = nullptr;
				triangles.vertexFormat = Utility::VoltToVulkanFormat(geometryInfo.vertexFormat);
				triangles.vertexData.deviceAddress = geometryInfo.vertexPositionsBuffer->GetDeviceAddress();
				triangles.vertexStride = geometryInfo.vertexStride;
				triangles.maxVertex = geometryInfo.vertexCount - 1;
				triangles.indexType = Utility::VoltToVulkanIndexType(geometryInfo.indexType);
				triangles.indexData.deviceAddress = geometryInfo.indexBuffer->GetDeviceAddress();
				triangles.transformData.deviceAddress = 0;

				primitiveCounts.emplace_back(geometryInfo.indexBuffer->GetCount() / 3u);
			}
			else if (geometryInfo.geometryType == AccelerationStructureGeometryType::Instances)
			{
				VT_ENSURE(geometryInfo.instancesBuffer);

				auto& instances = vulkanGeometry.geometry.instances;
				instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
				instances.pNext = nullptr;
				instances.arrayOfPointers = VK_FALSE;
				instances.data.deviceAddress = geometryInfo.instancesBuffer->GetDeviceAddress();

				primitiveCounts.emplace_back(geometryInfo.instancesBuffer->GetCount());
			}
		}

		buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildGeometryInfo.pNext = nullptr;
		buildGeometryInfo.type = Utility::GetAccelerationStructureType(createInfo.type);
		buildGeometryInfo.flags = Utility::GetAccelerationStructureBuildFlags(createInfo.flags);
		buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
		buildGeometryInfo.pGeometries = geometries.data();
		buildGeometryInfo.ppGeometries = nullptr;
		buildGeometryInfo.scratchData.deviceAddress = 0;
		buildGeometryInfo.srcAccelerationStructure = nullptr;
		buildGeometryInfo.dstAccelerationStructure = nullptr;

		VkAccelerationStructureBuildSizesInfoKHR buildSizes;
		buildSizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		buildSizes.pNext = nullptr;
		buildSizes.accelerationStructureSize = 0;
		buildSizes.updateScratchSize = 0;
		buildSizes.buildScratchSize = 0;

		auto device = GraphicsContext::GetDevice();
		vkGetAccelerationStructureBuildSizesKHR(device->GetHandle<VkDevice>(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo, primitiveCounts.data(), &buildSizes);
	
		m_backingBuffer = StorageBuffer::Create(1, buildSizes.accelerationStructureSize, "Acceleration Structure Backing Buffer", BufferUsage::AccelerationStructure | BufferUsage::DeviceAddress);
		
		VkAccelerationStructureCreateInfoKHR asCreateInfo{};
		asCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		asCreateInfo.pNext = nullptr;
		asCreateInfo.createFlags = 0;
		asCreateInfo.buffer = m_backingBuffer->GetHandle<VkBuffer>();
		asCreateInfo.offset = 0;
		asCreateInfo.size = buildSizes.accelerationStructureSize;
		asCreateInfo.type = Utility::GetAccelerationStructureType(createInfo.type);
		asCreateInfo.deviceAddress = 0;

		VT_VK_CHECK(vkCreateAccelerationStructureKHR(device->GetHandle<VkDevice>(), &asCreateInfo, nullptr, &m_handle));

		VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo{};
		deviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		deviceAddressInfo.pNext = nullptr;
		deviceAddressInfo.accelerationStructure = m_handle;

		m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device->GetHandle<VkDevice>(), &deviceAddressInfo);
	}
}
