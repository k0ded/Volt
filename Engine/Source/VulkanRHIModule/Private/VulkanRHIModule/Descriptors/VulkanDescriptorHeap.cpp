#include "vkpch.h"

#include "VulkanRHIModule/Descriptors/VulkanDescriptorHeap.h"
#include "VulkanRHIModule/Graphics/PhysicalDeviceProperties.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Memory/GPUAllocator.h>

#include <CoreUtilities/MemoryUtility.h>

#include <vulkan/vulkan_core.h>

namespace Volt::RHI
{
	// 1 MB per ring.
	constexpr uint64_t DescriptorRingBufferSize = (1ull << 24);

	VulkanDescriptorHeap::VulkanDescriptorHeap()
	{
		Initialize();
	}

	VulkanDescriptorHeap::~VulkanDescriptorHeap()
	{
		if (m_ringBufferState.mappedPtr)
		{
			m_descriptorHeapAllocation->Unmap();
			m_ringBufferState.mappedPtr = nullptr;
		}

		Release();
	}

	void VulkanDescriptorHeap::Initialize()
	{
		BufferDesc bufferDesc{};
		bufferDesc.numElements = 1;
		bufferDesc.elementSize = DescriptorRingBufferSize * RHICapabilities::NumFramesInFlight;
		bufferDesc.usage = BufferUsage::DescriptorBuffer | BufferUsage::DeviceAddress;
		bufferDesc.memoryUsage = MemoryUsage::CPUToGPU;
		bufferDesc.debugName = "DescriptorHeap";

		m_descriptorHeapAllocation = GraphicsContext::Get().GetDefaultAllocator()->CreateBuffer(bufferDesc);
	
		m_ringBufferState.head = 0;
		m_ringBufferState.size = DescriptorRingBufferSize;
		m_ringBufferState.mappedPtr = m_descriptorHeapAllocation->Map<uint8_t>(); // Keep mapped during application lifetime.
	}

	void VulkanDescriptorHeap::Release()
	{
		GraphicsContext::Get().GetDefaultAllocator()->DestroyBuffer(m_descriptorHeapAllocation);
	}

	void VulkanDescriptorHeap::BeginFrame()
	{
		m_frameIndex = (m_frameIndex + 1) % RHICapabilities::NumFramesInFlight;
		m_ringBufferState.head = 0;
	}

	uint64_t VulkanDescriptorHeap::AllocateDescriptorSet(uint64_t descriptorSetLayoutSize)
	{
		// Over allocate by the alignment, to make sure that we can align properly.
		const uint64_t totalSize = descriptorSetLayoutSize + g_physicalDeviceProperties.descriptorBufferProperties.descriptorBufferOffsetAlignment;
		uint64_t head = m_ringBufferState.head.fetch_add(totalSize, std::memory_order::relaxed);
		uint64_t alignedHead = ::Utility::Align(head, g_physicalDeviceProperties.descriptorBufferProperties.descriptorBufferOffsetAlignment);

		VT_ENSURE(alignedHead + descriptorSetLayoutSize < m_ringBufferState.size);
		return alignedHead;
	}

	uint8_t* VulkanDescriptorHeap::GetHeapPointer()
	{
		return &m_ringBufferState.mappedPtr[GetBaseOffset()];
	}

	uint64_t VulkanDescriptorHeap::GetDeviceAddress() const
	{
		return m_descriptorHeapAllocation->GetDeviceAddress();
	}

	uint64_t VulkanDescriptorHeap::GetBaseOffset() const
	{
		return m_frameIndex * DescriptorRingBufferSize;
	}

	void VulkanDescriptorHeap::Flush()
	{
		m_descriptorHeapAllocation->Flush(GetBaseOffset(), DescriptorRingBufferSize);
	}
}
