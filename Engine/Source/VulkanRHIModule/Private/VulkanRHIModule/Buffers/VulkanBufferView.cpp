#include "vkpch.h"

#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Buffers/VulkanStorageBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanUniformBuffer.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"

#include "VulkanRHIModule/Graphics/PhysicalDeviceProperties.h"

#include <RHIModule/Core/RHIResource.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/EnumUtils.h>
#include <CoreUtilities/Profiling/Profiling.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanBufferView::VulkanBufferView(const BufferViewDesc& desc, RawPtr<StorageBuffer> buffer)
		: m_desc(desc), m_resource(buffer)
	{
		CreateView();
	}

	VulkanBufferView::VulkanBufferView(const BufferViewDesc& desc, RawPtr<UniformBuffer> buffer)
		: m_desc(desc), m_resource(buffer)
	{
		CreateView();
	}

	VulkanBufferView::~VulkanBufferView()
	{
	}

	const uint64_t VulkanBufferView::GetDeviceAddress() const
	{
		return m_resource->GetDeviceAddress();
	}

	void* VulkanBufferView::GetHandleImpl() const
	{
		return m_resource->GetHandle<void*>();
	}

	bool VulkanBufferView::IsTexelBufferView() const
	{
		return m_desc.bufferFormat != PixelFormat::UNDEFINED;
	}

	void VulkanBufferView::CreateView()
	{
		VT_PROFILE_FUNCTION();

		uint64_t maxRange = 0;
		if (m_resource->GetType() == ResourceType::StorageBuffer)
		{
			VulkanStorageBuffer& vkStorageBuffer = m_resource->AsRef<VulkanStorageBuffer>();
			maxRange = std::min(vkStorageBuffer.GetCount() * vkStorageBuffer.GetElementSize(), m_desc.size);
		}
		else if (m_resource->GetType() == ResourceType::UniformBuffer)
		{
			VulkanUniformBuffer& vkUniformBuffer = m_resource->AsRef<VulkanUniformBuffer>();
			maxRange = std::min((uint64_t)vkUniformBuffer.GetSize(), m_desc.size);
		}

		memset(&m_srvDescriptor, 0, sizeof(m_srvDescriptor));
		memset(&m_uavDescriptor, 0, sizeof(m_uavDescriptor));

		// All buffer views has the same address info.
		m_srvDescriptor.vkDescriptorInfo.sType = m_uavDescriptor.vkDescriptorInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
		m_srvDescriptor.vkDescriptorInfo.pNext = m_uavDescriptor.vkDescriptorInfo.pNext = nullptr;

		m_srvDescriptor.addressInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT;
		m_srvDescriptor.addressInfo.pNext = nullptr;
		m_srvDescriptor.addressInfo.address = GetDeviceAddress() + m_desc.offset;
		m_srvDescriptor.addressInfo.range = maxRange;
		m_srvDescriptor.addressInfo.format = Utility::VoltToVulkanFormat(m_desc.bufferFormat);

		// SRV and UAV descriptors have the same address info.
		m_uavDescriptor.addressInfo = m_srvDescriptor.addressInfo;

		if (m_resource->GetType() == ResourceType::StorageBuffer)
		{
			if (IsTexelBufferView())
			{
				m_srvDescriptor.vkDescriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
				m_srvDescriptor.vkDescriptorInfo.data.pUniformTexelBuffer = &m_srvDescriptor.addressInfo;

				m_uavDescriptor.vkDescriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
				m_uavDescriptor.vkDescriptorInfo.data.pStorageTexelBuffer = &m_uavDescriptor.addressInfo;

				m_srvDescriptor.descriptorSize = g_physicalDeviceProperties.descriptorBufferProperties.uniformTexelBufferDescriptorSize;
				m_uavDescriptor.descriptorSize = g_physicalDeviceProperties.descriptorBufferProperties.storageTexelBufferDescriptorSize;
			}
			else
			{
				m_srvDescriptor.vkDescriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				m_srvDescriptor.vkDescriptorInfo.data.pStorageBuffer = &m_srvDescriptor.addressInfo;

				m_uavDescriptor.vkDescriptorInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				m_uavDescriptor.vkDescriptorInfo.data.pStorageBuffer = &m_uavDescriptor.addressInfo;

				m_srvDescriptor.descriptorSize = g_physicalDeviceProperties.descriptorBufferProperties.storageBufferDescriptorSize;
				m_uavDescriptor.descriptorSize = g_physicalDeviceProperties.descriptorBufferProperties.storageBufferDescriptorSize;
			}
		}
		else if (m_resource->GetType() == ResourceType::UniformBuffer)
		{
			// No UAV descriptor is created for uniform buffers.
			m_srvDescriptor.vkDescriptorInfo.data.pUniformBuffer = &m_srvDescriptor.addressInfo;
			m_srvDescriptor.vkDescriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			m_srvDescriptor.descriptorSize = g_physicalDeviceProperties.descriptorBufferProperties.uniformBufferDescriptorSize;
		}
	}
}
