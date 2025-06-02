#include "vkpch.h"

#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Buffers/VulkanStorageBuffer.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"

#include <RHIModule/Core/RHIResource.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/EnumUtils.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanBufferView::VulkanBufferView(const BufferViewDesc& specification)
		: m_buffer(specification.bufferResource)
	{
		// If the resource is a storage buffer, we nned to check if it's
		// a texel buffer, and create a VkBufferView if that's the case.
		if (specification.bufferResource->GetType() == ResourceType::StorageBuffer)
		{
			VulkanStorageBuffer& vkStorageBuffer = specification.bufferResource->AsRef<VulkanStorageBuffer>();
			const BufferDesc& bufferDesc = vkStorageBuffer.GetDesc();

			if (EnumValueContainsFlag(bufferDesc.usage, BufferUsage::TexelBuffer))
			{
				VkBufferViewCreateInfo viewCreateInfo;
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
				viewCreateInfo.pNext = nullptr;
				viewCreateInfo.flags = 0;
				viewCreateInfo.buffer = vkStorageBuffer.GetHandle<VkBuffer>();
				viewCreateInfo.format = Utility::VoltToVulkanFormat(specification.bufferFormat);
				viewCreateInfo.offset = specification.offset;
				viewCreateInfo.range = specification.size;

				auto device = GraphicsContext::GetDevice();
				vkCreateBufferView(device->GetHandle<VkDevice>(), &viewCreateInfo, nullptr, &m_texelBufferView);
			}
		}
	}

	VulkanBufferView::~VulkanBufferView()
	{
		if (m_texelBufferView)
		{
			RHIModule::GetInstance().DestroyResource([view = m_texelBufferView]() 
			{
				auto device = GraphicsContext::GetDevice();
				vkDestroyBufferView(device->GetHandle<VkDevice>(), view, nullptr);
			});
		}
	}

	const uint64_t VulkanBufferView::GetDeviceAddress() const
	{
		return m_buffer->GetDeviceAddress();
	}

	void* VulkanBufferView::GetHandleImpl() const
	{
		return m_buffer->GetHandle<void*>();
	}

	bool VulkanBufferView::IsTexelBufferView() const
	{
		return m_texelBufferView != nullptr;
	}
}
