#include "vkpch.h"
#include "VulkanRHIModule/Buffers/VulkanIndexBuffer.h"

#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>

#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/RHIModule.h>

namespace Volt::RHI
{
	VulkanIndexBuffer::VulkanIndexBuffer(std::span<const uint32_t> indices)
		: m_count(static_cast<uint32_t>(indices.size()))
	{
		GraphicsContext::GetResourceStateTracker()->AddResource(this, BarrierStage::None, BarrierAccess::None);
		SetData(indices.data(), static_cast<uint32_t>(sizeof(uint32_t) * indices.size()));
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		GraphicsContext::GetResourceStateTracker()->RemoveResource(this);

		if (!m_allocation)
		{
			return;
		}

		GraphicsContext::GetDefaultAllocator()->DestroyBuffer(m_allocation);
		m_allocation = nullptr;
	}

	const uint32_t VulkanIndexBuffer::GetCount() const
	{
		return m_count;
	}

	void VulkanIndexBuffer::SetName(const std::string& name)
	{
		if (Volt::RHI::vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
			nameInfo.objectHandle = (uint64_t)m_allocation->GetResourceHandle<VkBuffer>();
			nameInfo.pObjectName = name.c_str();

			auto device = GraphicsContext::GetDevice();
			Volt::RHI::vkSetDebugUtilsObjectNameEXT(device->GetHandle<VkDevice>(), &nameInfo);
		}

		m_name = name;
	}

	std::string_view VulkanIndexBuffer::GetName() const
	{
		return m_name;
	}

	const uint64_t VulkanIndexBuffer::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	const uint64_t VulkanIndexBuffer::GetByteSize() const
	{
		return m_allocation->GetSize();
	}

	void* VulkanIndexBuffer::GetHandleImpl() const
	{
		return m_allocation->GetResourceHandle<VkBuffer>();
	}

	void VulkanIndexBuffer::SetData(const void* data, const uint32_t size)
	{
		VkDeviceSize bufferSize = size;

		Handle<Allocation> stagingAllocation;

		if (m_allocation)
		{
			GraphicsContext::GetDefaultAllocator()->DestroyBuffer(m_allocation);
			m_allocation = nullptr;
		}

		auto allocator = GraphicsContext::GetDefaultAllocator();

		if (data)
		{
			BufferDesc stagingDesc{};
			stagingDesc.count = 1;
			stagingDesc.elementSize = bufferSize;
			stagingDesc.usage = BufferUsage::TransferSrc;
			stagingDesc.memoryUsage = MemoryUsage::CPU;
			stagingDesc.debugName = "Staging Alloc";

			stagingAllocation = allocator->CreateBuffer(stagingDesc);

			// Copy to staging buffer
			{
				void* buffData = stagingAllocation->Map<void>();
				memcpy_s(buffData, size, data, size);
				stagingAllocation->Unmap();
			}
		}

		// Create GPU buffer
		{
			BufferDesc desc{};
			desc.count = 1;
			desc.elementSize = bufferSize;
			desc.usage = BufferUsage::IndexBuffer | BufferUsage::TransferDst;
			desc.memoryUsage = MemoryUsage::CPU;
			desc.debugName = m_name;

			m_allocation = allocator->CreateBuffer(desc);
		}

		if (data)
		{
			// Copy from staging buffer to GPU buffer
			{
				RefPtr<CommandBuffer> cmdBuffer = CommandBuffer::Create();
				cmdBuffer->Begin();

				VkBufferCopy copy{};
				copy.srcOffset = 0;
				copy.dstOffset = 0;
				copy.size = bufferSize;

				vkCmdCopyBuffer(cmdBuffer->GetHandle<VkCommandBuffer>(), stagingAllocation->GetResourceHandle<VkBuffer>(), m_allocation->GetResourceHandle<VkBuffer>(), 1, &copy);

				cmdBuffer->End();

				CommandBufferUtils::ExecuteCommandBufferWithNewFence(cmdBuffer);
			}

			allocator->DestroyBuffer(stagingAllocation);
		}
	}
}
