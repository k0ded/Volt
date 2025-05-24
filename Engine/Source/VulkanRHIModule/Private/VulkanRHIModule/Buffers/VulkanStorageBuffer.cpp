#include "vkpch.h"
#include "VulkanRHIModule/Buffers/VulkanStorageBuffer.h"

#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Buffers/BufferView.h>
#include <RHIModule/Buffers/CommandBuffer.h>

#include <RHIModule/Memory/MemoryCommon.h>
#include <RHIModule/Memory/Allocation.h>

#include <RHIModule/RHIModule.h>

namespace Volt::RHI
{
	VulkanStorageBuffer::VulkanStorageBuffer(uint32_t count, uint64_t elementSize, const std::string& name, BufferUsage bufferUsage, MemoryUsage memoryUsage, RefPtr<GPUAllocator> allocator)
		: m_elementSize(elementSize), m_count(count), m_name(name), m_allocator(allocator), m_bufferUsage(bufferUsage), m_memoryUsage(memoryUsage)
	{
		GraphicsContext::GetResourceStateTracker()->AddResource(this, BarrierStage::None, BarrierAccess::None);

		if (!m_allocator)
		{
			m_allocator = GraphicsContext::GetDefaultAllocator();
		}

		Invalidate(m_elementSize * m_count);
		SetName(m_name);
	}

	VulkanStorageBuffer::~VulkanStorageBuffer()
	{
		GraphicsContext::GetResourceStateTracker()->RemoveResource(this);
		Release();
	}

	void VulkanStorageBuffer::Resize(const uint64_t byteSize)
	{
		m_count = static_cast<uint32_t>(byteSize);

		Invalidate(byteSize);
		SetName(m_name);
	}

	void VulkanStorageBuffer::ResizeWithCount(const uint32_t count)
	{
		auto oldAllocation = m_allocation;
		auto oldSize = m_byteSize;

		m_count = count;
		const uint64_t newSize = m_count * m_elementSize;

		// We don't need to do a full recreate if the current buffer can hold the requested count.
		if (newSize <= m_allocation->GetSize())
		{
			return;
		}

		Release();

		m_byteSize = std::max(newSize, Memory::GetMinBufferAllocationSize());

		const VkDeviceSize bufferSize = m_byteSize;
		m_allocation = m_allocator->CreateBuffer(bufferSize, m_bufferUsage | BufferUsage::TransferDst | BufferUsage::TransferSrc | BufferUsage::StorageBuffer, m_memoryUsage, m_name);

		SetName(m_name);

		// Copy old data to new buffer
		RefPtr<CommandBuffer> commandBuffer = CommandBuffer::Create();

		commandBuffer->Begin();

		ResourceBarrierInfo barrierInfo{};
		barrierInfo.type = BarrierType::Global;
		barrierInfo.globalBarrier().srcStage = BarrierStage::All;
		barrierInfo.globalBarrier().dstStage = BarrierStage::Copy;
		barrierInfo.globalBarrier().srcAccess = BarrierAccess::None;
		barrierInfo.globalBarrier().dstAccess = BarrierAccess::CopyDest | BarrierAccess::CopySource;

		commandBuffer->ResourceBarrier({ barrierInfo });
		commandBuffer->CopyBufferRegion(oldAllocation, 0, m_allocation, 0, oldSize);

		barrierInfo.globalBarrier().srcStage = BarrierStage::Copy;
		barrierInfo.globalBarrier().dstStage = BarrierStage::All;
		barrierInfo.globalBarrier().srcAccess = BarrierAccess::CopyDest | BarrierAccess::CopySource;
		barrierInfo.globalBarrier().dstAccess = BarrierAccess::None;

		commandBuffer->ResourceBarrier({ barrierInfo });

		commandBuffer->End();
		commandBuffer->Execute();
	}

	const size_t VulkanStorageBuffer::GetByteSize() const
	{
		return m_allocation->GetSize();
	}

	const size_t VulkanStorageBuffer::GetElementSize() const
	{
		return m_elementSize;
	}

	const uint32_t VulkanStorageBuffer::GetCount() const
	{
		return m_count;
	}

	Handle<Allocation> VulkanStorageBuffer::GetAllocation() const
	{
		return m_allocation;
	}

	void VulkanStorageBuffer::Unmap()
	{
		m_allocation->Unmap();
	}

	void VulkanStorageBuffer::SetData(const void* data, const size_t size)
	{
		Handle<Allocation> stagingAllocation = GraphicsContext::GetDefaultAllocator()->CreateBuffer(size, BufferUsage::TransferSrc, MemoryUsage::CPUToGPU, "Staging Alloc");

		void* mappedPtr = stagingAllocation->Map<void>();
		memcpy_s(mappedPtr, size, data, size);
		stagingAllocation->Unmap();

		RefPtr<CommandBuffer> cmdBuffer = CommandBuffer::Create();
		cmdBuffer->Begin();

		ResourceBarrierInfo barrier{};
		barrier.type = BarrierType::Buffer;
		barrier.bufferBarrier().srcStage = BarrierStage::ComputeShader | BarrierStage::VertexShader | BarrierStage::PixelShader;
		barrier.bufferBarrier().srcAccess = BarrierAccess::None;
		barrier.bufferBarrier().dstStage = BarrierStage::Copy;
		barrier.bufferBarrier().dstAccess = BarrierAccess::CopyDest;
		barrier.bufferBarrier().offset = 0;
		barrier.bufferBarrier().size = m_allocation->GetSize();
		barrier.bufferBarrier().resource = RawPtr<VulkanStorageBuffer>(this);

		cmdBuffer->ResourceBarrier({ barrier });

		cmdBuffer->CopyBufferRegion(stagingAllocation, 0, m_allocation, 0, size);

		barrier.bufferBarrier().srcStage = BarrierStage::Copy;
		barrier.bufferBarrier().srcAccess = BarrierAccess::CopyDest;
		barrier.bufferBarrier().dstStage = BarrierStage::ComputeShader | BarrierStage::VertexShader | BarrierStage::PixelShader;
		barrier.bufferBarrier().dstAccess = BarrierAccess::None;

		cmdBuffer->ResourceBarrier({ barrier });

		cmdBuffer->End();
		cmdBuffer->Execute();

		GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAllocation);
	}

	void VulkanStorageBuffer::SetData(RefPtr<CommandBuffer> commandBuffer, const void* data, const size_t size)
	{
		Handle<Allocation> stagingAllocation = GraphicsContext::GetDefaultAllocator()->CreateBuffer(size, BufferUsage::TransferSrc, MemoryUsage::CPUToGPU, "Staging Alloc");

		void* mappedPtr = stagingAllocation->Map<void>();
		memcpy_s(mappedPtr, m_byteSize, data, size);
		stagingAllocation->Unmap();

		ResourceBarrierInfo barrier{};
		barrier.type = BarrierType::Buffer;
		barrier.bufferBarrier().srcStage = BarrierStage::All;
		barrier.bufferBarrier().srcAccess = BarrierAccess::None;
		barrier.bufferBarrier().dstStage = BarrierStage::Copy;
		barrier.bufferBarrier().dstAccess = BarrierAccess::CopyDest;
		barrier.bufferBarrier().offset = 0;
		barrier.bufferBarrier().size = size;
		barrier.bufferBarrier().resource = RawPtr<VulkanStorageBuffer>(this);

		commandBuffer->ResourceBarrier({ barrier });

		commandBuffer->CopyBufferRegion(stagingAllocation, 0, m_allocation, 0, size);

		barrier.bufferBarrier().srcStage = BarrierStage::Copy;
		barrier.bufferBarrier().srcAccess = BarrierAccess::CopyDest;
		barrier.bufferBarrier().dstStage = BarrierStage::All;
		barrier.bufferBarrier().dstAccess = BarrierAccess::None;

		commandBuffer->ResourceBarrier({ barrier });

		GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAllocation);
	}

	RefPtr<BufferView> VulkanStorageBuffer::GetView()
	{
		if (m_view)
		{
			return m_view;
		}

		BufferViewSpecification spec{};
		spec.bufferResource = this;

		m_view = BufferView::Create(spec);
		return m_view;
	}

	void VulkanStorageBuffer::SetName(const std::string& name)
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

	std::string_view VulkanStorageBuffer::GetName() const
	{
		return m_name;
	}

	const uint64_t VulkanStorageBuffer::GetDeviceAddress() const
	{
		return m_allocation->GetDeviceAddress();
	}

	void* VulkanStorageBuffer::GetHandleImpl() const
	{
		return m_allocation->GetResourceHandle<VkBuffer>();
	}

	void* VulkanStorageBuffer::MapInternal()
	{
		return m_allocation->Map<void>();
	}

	void VulkanStorageBuffer::Invalidate(const uint64_t byteSize)
	{
		Release();
		m_byteSize = std::max(byteSize, Memory::GetMinBufferAllocationSize());

		const VkDeviceSize bufferSize = m_byteSize;
		m_allocation = m_allocator->CreateBuffer(bufferSize, m_bufferUsage | BufferUsage::TransferDst | BufferUsage::TransferSrc | BufferUsage::StorageBuffer, m_memoryUsage, m_name);
	}

	void VulkanStorageBuffer::Release()
	{
		if (!m_allocation)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([allocator = m_allocator, allocation = m_allocation]() 
		{
			allocator->DestroyBuffer(allocation);
		});
		m_allocation = nullptr;
	}
}
