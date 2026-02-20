#include "vkpch.h"

#include "VulkanRHIModule/Buffers/VulkanTransientBuffer.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Memory/Allocation.h>

#include <CoreUtilities/EnumUtils.h>

namespace Volt::RHI
{

	VulkanTransientBuffer::VulkanTransientBuffer(const BufferDesc& desc)
		: m_desc(desc)
	{
		GraphicsContext::GetResourceStateTracker()->AddResource(this, BarrierStage::None, BarrierAccess::None);

		// Make sure that the desc contains either storage buffer or texel buffer.
		if (!EnumValueContainsFlag(m_desc.usage, BufferUsage::StorageBuffer) && !EnumValueContainsFlag(m_desc.usage, BufferUsage::TexelBuffer))
		{
			m_desc.usage |= BufferUsage::StorageBuffer;
		}

		// Make sure buffer always contains transfer source and transfer dest
		m_desc.usage |= BufferUsage::TransferSrc | BufferUsage::TransferDst | BufferUsage::DeviceAddress;

		CreateBuffer();
		SetName(desc.debugName);
	}

	VulkanTransientBuffer::~VulkanTransientBuffer()
	{
		GraphicsContext::GetResourceStateTracker()->RemoveResource(this);

		if (m_allocation)
		{
			GraphicsContext::GetTransientAllocator()->DestroyBuffer(m_allocation);
		}
	}

	const BufferDesc& VulkanTransientBuffer::GetDesc() const
	{
		return m_desc;
	}

	uint64_t VulkanTransientBuffer::GetElementSize() const
	{
		return m_desc.elementSize;
	}

	uint64_t VulkanTransientBuffer::GetNumElements() const
	{
		return m_desc.numElements;
	}

	RefPtr<BufferView> VulkanTransientBuffer::GetView(const BufferViewDesc& desc)
	{
		RefPtr<BufferView> bufferView = BufferView::Create(desc, this);
		return bufferView;
	}

	void VulkanTransientBuffer::Unmap()
	{
		m_allocation->Unmap();
	}

	void VulkanTransientBuffer::SetName(const std::string & name)
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

	std::string_view VulkanTransientBuffer::GetName() const
	{
		return m_desc.debugName;
	}

	uint64_t VulkanTransientBuffer::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	const MemoryRequirement& VulkanTransientBuffer::GetMemoryRequirements() const
	{
		return m_allocation->GetMemoryRequirements();
	}

	void* VulkanTransientBuffer::GetHandleImpl() const
	{
		return m_allocation->GetResourceHandle<VkBuffer>();
	}

	void* VulkanTransientBuffer::MapInternal()
	{
		return m_allocation->Map<void>();
	}

	void VulkanTransientBuffer::CreateBuffer()
	{
		m_allocation = GraphicsContext::GetTransientAllocator()->CreateBuffer(m_desc);
	}
}
