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
	VulkanUniformBuffer::VulkanUniformBuffer(const UniformBufferDesc& desc, const void* initialData)
		: m_desc(desc)
	{
		m_resourceStateTracker.Initialize(this, BarrierStage::None, BarrierAccess::None);

		const auto& deviceProperties = GraphicsContext::GetPhysicalDevice()->As<VulkanPhysicalGraphicsDevice>()->GetProperties();
		
		VT_ENSURE_MSG(desc.size % 16u == 0, "UniformBuffers must be 16 byte aligned!");
		const uint64_t alignedSize = Utility::Align(desc.size, deviceProperties.limits.minUniformBufferOffsetAlignment);

		BufferDesc bufferDesc{};
		bufferDesc.numElements = 1;
		bufferDesc.elementSize = alignedSize;
		bufferDesc.usage = BufferUsage::UniformBuffer | BufferUsage::DeviceAddress;
		bufferDesc.memoryUsage = MemoryUsage::CPUToGPU;
		bufferDesc.debugName = desc.debugName;

		m_allocation = GraphicsContext::GetDefaultAllocator()->CreateBuffer(bufferDesc);

		if (initialData)
		{
			void* mappedData = MapInternal();
			memcpy_s(mappedData, desc.size, initialData, desc.size);
			Unmap();
		}

		SetName(desc.debugName);
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
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
		return BufferView::Create(desc, this);
	}

	uint64_t VulkanUniformBuffer::GetSize() const
	{
		return m_desc.size;
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

		m_desc.debugName = name;
	}

	std::string_view VulkanUniformBuffer::GetName() const
	{
		return m_desc.debugName;
	}

	uint64_t VulkanUniformBuffer::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	void* VulkanUniformBuffer::MapInternal()
	{
		uint8_t* bytePtr = m_allocation->Map<uint8_t>();
		return bytePtr;
	}

	void* VulkanUniformBuffer::GetHandleImpl() const
	{
		return m_allocation->GetResourceHandle<VkBuffer>();
	}

	const MemoryRequirement& VulkanUniformBuffer::GetMemoryRequirements() const
	{
		return m_allocation->GetMemoryRequirements();
	}

	uint64_t VulkanUniformBuffer::GetResourceByteSize() const
	{
		return m_desc.size;
	}
}
