#include "vkpch.h"
#include "VulkanRHIModule/Buffers/VulkanUniformBuffer.h"

#include "VulkanRHIModule/Common/VulkanFunctions.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"

#include <RHIModule/Buffers/BufferView.h>

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>

#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/Memory/MemoryUtility.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt::RHI
{
	VulkanUniformBuffer::VulkanUniformBuffer(const uint32_t size, const void* data, const uint32_t count, const std::string& name)
		: m_size(size), m_name(name)
	{
		VT_PROFILE_FUNCTION();

		GraphicsContext::GetResourceStateTracker()->AddResource(this, BarrierStage::None, BarrierAccess::None);

		const auto& deviceProperties = GraphicsContext::GetPhysicalDevice()->As<VulkanPhysicalGraphicsDevice>()->GetProperties();
		const uint64_t alignedSize = Utility::Align(size, deviceProperties.limits.minUniformBufferOffsetAlignment);

		BufferDesc desc{};
		desc.count = count;
		desc.elementSize = alignedSize;
		desc.usage = BufferUsage::UniformBuffer;
		desc.memoryUsage = MemoryUsage::CPUToGPU;
		desc.debugName = m_name;

		m_allocation = GraphicsContext::GetDefaultAllocator()->CreateBuffer(desc);

		if (data)
		{
			SetData(data, size);
		}

		SetName(name);
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		GraphicsContext::GetResourceStateTracker()->RemoveResource(this);

		if (m_allocation == nullptr)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([allocation = m_allocation]() 
		{
			GraphicsContext::GetDefaultAllocator()->DestroyBuffer(allocation);
		});

		m_allocation = nullptr;
	}

	RefPtr<BufferView> VulkanUniformBuffer::GetView(const BufferViewDesc& desc)
	{
		BufferViewDesc descCopy = desc;
		descCopy.bufferResource = this;

		return BufferView::Create(descCopy);
	}

	const uint32_t VulkanUniformBuffer::GetSize() const
	{
		const auto& deviceProperties = GraphicsContext::GetPhysicalDevice()->As<VulkanPhysicalGraphicsDevice>()->GetProperties();
		return Utility::Align(m_size, deviceProperties.limits.minUniformBufferOffsetAlignment);
	}

	void VulkanUniformBuffer::SetData(const void* data, const uint32_t size)
	{
		void* bufferData = m_allocation->Map<void>();
		memcpy_s(bufferData, m_size, data, size);
		m_allocation->Unmap();
	}

	void VulkanUniformBuffer::Unmap()
	{
		m_allocation->Unmap();
	}

	void VulkanUniformBuffer::SetName(const std::string& name)
	{
		if (Volt::RHI::vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
			nameInfo.objectHandle = (uint64_t)m_allocation->GetResourceHandle<VkBuffer>();
			nameInfo.pObjectName = name.data();

			auto device = GraphicsContext::GetDevice();
			Volt::RHI::vkSetDebugUtilsObjectNameEXT(device->GetHandle<VkDevice>(), &nameInfo);
		}

		m_name = name;
	}

	std::string_view VulkanUniformBuffer::GetName() const
	{
		return m_name;
	}

	const uint64_t VulkanUniformBuffer::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	const uint64_t VulkanUniformBuffer::GetByteSize() const
	{
		return m_allocation->GetSize();
	}

	void* VulkanUniformBuffer::MapInternal(const uint32_t index)
	{
		const auto& deviceProperties = GraphicsContext::GetPhysicalDevice()->As<VulkanPhysicalGraphicsDevice>()->GetProperties();
		const uint32_t offset = Utility::Align(m_size, deviceProperties.limits.minUniformBufferOffsetAlignment) * index;

		uint8_t* bytePtr = m_allocation->Map<uint8_t>();
		return &bytePtr[offset];
	}

	void* VulkanUniformBuffer::GetHandleImpl() const
	{
		return m_allocation->GetResourceHandle<VkBuffer>();
	}
}
