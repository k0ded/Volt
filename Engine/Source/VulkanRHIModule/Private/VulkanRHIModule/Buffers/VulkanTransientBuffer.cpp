#include "vkpch.h"

#include "VulkanRHIModule/Buffers/VulkanTransientBuffer.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"
#include "VulkanRHIModule/VulkanResourceCast.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/EnumUtils.h>

namespace Volt::RHI
{

	VulkanTransientBuffer::VulkanTransientBuffer(const BufferDesc& desc)
		: m_desc(desc),
		m_bufferHandle(nullptr)
	{
		m_resourceStateTracker.Initialize(this, BarrierStage::None, BarrierAccess::None);

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
		if (m_bufferHandle)
		{
			RHIModule::GetInstance().DestroyResource([bufferHandle = m_bufferHandle]()
			{
				auto device = GraphicsContext::GetDevice();
				vkDestroyBuffer(device->GetHandle<VkDevice>(), bufferHandle, VT_VULKAN_ALLOCATOR);
			}, GetLastSubmissionTrackerFence());
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

	IntRef<BufferView> VulkanTransientBuffer::GetView(const BufferViewDesc& desc)
	{
		IntRef<BufferView> bufferView = BufferView::Create(desc, this);
		return bufferView;
	}

	void VulkanTransientBuffer::Unmap()
	{
		VT_ENSURE_NO_ENTRY();
	}

	void VulkanTransientBuffer::SetName(const String & name)
	{
		if (Volt::RHI::vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
			nameInfo.objectHandle = (uint64_t)m_bufferHandle;
			nameInfo.pObjectName = name.data();

			auto device = GraphicsContext::GetDevice();
			Volt::RHI::vkSetDebugUtilsObjectNameEXT(device->GetHandle<VkDevice>(), &nameInfo);
		}

		m_desc.debugName = name;
	}

	StringView VulkanTransientBuffer::GetName() const
	{
		return m_desc.debugName;
	}

	uint64_t VulkanTransientBuffer::GetDeviceAddress() const
	{
		return m_deviceAddress;
	}

	const MemoryRequirement& VulkanTransientBuffer::GetMemoryRequirements() const
	{
		return m_memoryRequirements;
	}

	void* VulkanTransientBuffer::GetHandleImpl() const
	{
		return m_bufferHandle;
	}

	void* VulkanTransientBuffer::MapInternal()
	{
		VT_ENSURE_NO_ENTRY();
		return nullptr;
	}

	void VulkanTransientBuffer::CreateBuffer()
	{
		// Create buffer object
		{
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.pNext = nullptr;
			bufferInfo.pQueueFamilyIndices = nullptr;
			bufferInfo.queueFamilyIndexCount = 0;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			bufferInfo.size = m_desc.elementSize * m_desc.numElements;
			bufferInfo.usage = Utility::GetVkBufferUsageFlags(m_desc.usage);

			m_memoryRequirements = Utility::GetBufferMemoryRequirement(bufferInfo);

			auto device = GraphicsContext::GetDevice();

			vkCreateBuffer(device->GetHandle<VkDevice>(), &bufferInfo, VT_VULKAN_ALLOCATOR, &m_bufferHandle);
		}
	}

	uint64_t VulkanTransientBuffer::GetResourceByteSize() const
	{
		return m_desc.elementSize * m_desc.numElements;
	}

	void VulkanTransientBuffer::BindMemory(IntRef<RHI::TransientHeap> heap, uint32_t pageIndex, uint64_t offset)
	{
		auto device = GraphicsContext::GetDevice();

		IntRef<RHI::VulkanTransientHeap> vkHeap = ResourceCast(heap);
		vkBindBufferMemory(device->GetHandle<VkDevice>(), m_bufferHandle, vkHeap->GetPageMemoryHandle(pageIndex), offset);

		VkBufferDeviceAddressInfo deviceAddressInfo{};
		deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		deviceAddressInfo.pNext = nullptr;
		deviceAddressInfo.buffer = m_bufferHandle;

		m_deviceAddress = vkGetBufferDeviceAddress(device->GetHandle<VkDevice>(), &deviceAddressInfo);
	}
}
