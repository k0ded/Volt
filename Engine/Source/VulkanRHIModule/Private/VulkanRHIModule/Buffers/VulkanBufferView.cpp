#include "vkpch.h"

#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Buffers/VulkanStorageBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanUniformBuffer.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"

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
		if (m_texelBufferView)
		{
			RHIModule::GetInstance().DestroyResource([view = m_texelBufferView]() 
			{
				auto device = GraphicsContext::GetDevice();
				vkDestroyBufferView(device->GetHandle<VkDevice>(), view, VT_VULKAN_ALLOCATOR);
			});
		}
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
		return m_texelBufferView != nullptr;
	}

	void VulkanBufferView::CreateView()
	{
		VT_PROFILE_FUNCTION();

		// If the resource is a storage buffer, we nned to check if it's
		// a texel buffer, and create a VkBufferView if that's the case.
		if (m_resource->GetType() == ResourceType::StorageBuffer)
		{
			VulkanStorageBuffer& vkStorageBuffer = m_resource->AsRef<VulkanStorageBuffer>();
			const BufferDesc& bufferDesc = vkStorageBuffer.GetDesc();

			if (EnumValueContainsFlag(bufferDesc.usage, BufferUsage::TexelBuffer))
			{
				VkBufferViewCreateInfo viewCreateInfo;
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
				viewCreateInfo.pNext = nullptr;
				viewCreateInfo.flags = 0;
				viewCreateInfo.buffer = vkStorageBuffer.GetHandle<VkBuffer>();
				viewCreateInfo.format = Utility::VoltToVulkanFormat(m_desc.bufferFormat);
				viewCreateInfo.offset = m_desc.offset;
				viewCreateInfo.range = m_desc.size;

				auto device = GraphicsContext::GetDevice();
				{
					vkCreateBufferView(device->GetHandle<VkDevice>(), &viewCreateInfo, VT_VULKAN_ALLOCATOR, &m_texelBufferView);
				}
			}
		}
	}
}
