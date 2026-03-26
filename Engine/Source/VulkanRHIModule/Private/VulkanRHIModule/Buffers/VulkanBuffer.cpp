#include "vkpch.h"
#include "VulkanRHIModule/Buffers/VulkanBuffer.h"

#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Buffers/BufferView.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>

#include <RHIModule/Memory/MemoryCommon.h>
#include <RHIModule/Memory/Allocation.h>

#include <RHIModule/RHIModule.h>

#include <CoreUtilities/EnumUtils.h>
#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt::RHI
{
	VulkanBuffer::VulkanBuffer(const BufferDesc& desc)
		: m_desc(desc)
	{
		m_resourceStateTracker.Initialize(this, BarrierStage::None, BarrierAccess::None);

		// Make sure that the desc contains either storage buffer or texel buffer.
		if (!EnumValueContainsFlag(m_desc.usage, BufferUsage::StorageBuffer) && ! EnumValueContainsFlag(m_desc.usage, BufferUsage::TexelBuffer))
		{
			m_desc.usage |= BufferUsage::StorageBuffer;
		}

		// Make sure buffer always contains transfer source and transfer dest
		m_desc.usage |= BufferUsage::TransferSrc | BufferUsage::TransferDst | BufferUsage::DeviceAddress;

		Invalidate(desc.elementSize * desc.numElements);
		SetName(desc.debugName);
	}

	VulkanBuffer::~VulkanBuffer()
	{
		Release();
	}

	uint64_t VulkanBuffer::GetElementSize() const
	{
		return m_desc.elementSize;
	}

	uint64_t VulkanBuffer::GetNumElements() const
	{
		return m_desc.numElements;
	}

	void VulkanBuffer::Unmap()
	{
		m_allocation->Unmap();
	}

	IntRef<BufferView> VulkanBuffer::GetView(const BufferViewDesc& desc)
	{
		IntRef<BufferView> bufferView = BufferView::Create(desc, this);
		return bufferView;
	}

	void VulkanBuffer::SetName(const String& name)
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

	StringView VulkanBuffer::GetName() const
	{
		return m_desc.debugName;
	}

	uint64_t VulkanBuffer::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	void* VulkanBuffer::GetHandleImpl() const
	{
		return m_allocation->GetResourceHandle<VkBuffer>();
	}

	void* VulkanBuffer::MapInternal()
	{
		return m_allocation->Map<void>();
	}

	void VulkanBuffer::Invalidate(const uint64_t byteSize)
	{
		Release();
		m_byteSize = byteSize;
		m_allocation = GraphicsContext::GetDefaultAllocator()->CreateBuffer(m_desc);
	}

	void VulkanBuffer::Release()
	{
		if (!m_allocation)
		{
			return;
		}

		GraphicsContext::GetDefaultAllocator()->DestroyBuffer(m_allocation);
		m_allocation = nullptr;
	}

	const BufferDesc& VulkanBuffer::GetDesc() const
	{
		return m_desc;
	}

	const MemoryRequirement& VulkanBuffer::GetMemoryRequirements() const
	{
		return m_allocation->GetMemoryRequirements();
	}

	uint64_t VulkanBuffer::GetResourceByteSize() const
	{
		return m_byteSize;
	}
}
