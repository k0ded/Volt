#include "vkpch.h"

#include "VulkanRHIModule/Descriptors/VulkanResourceTable.h"
#include "VulkanRHIModule/Descriptors/ResourceTableDescriptorSetManager.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"
#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Images/VulkanImageView.h"
#include "VulkanRHIModule/Graphics/PhysicalDeviceProperties.h"

#include <RHIModule/Graphics/Swapchain.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIModule.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanResourceTable::VulkanResourceTable()
	{
		Initialize();
	}

	VulkanResourceTable::~VulkanResourceTable()
	{
		Release();
	}

	void VulkanResourceTable::AddBuffer(RefPtr<Buffer> buffer)
	{
		m_bufferTable.Add(buffer);
	}

	void VulkanResourceTable::AddTexture(RefPtr<Image> texture)
	{
		m_textureTable.Add(texture);
	}

	void VulkanResourceTable::RemoveBuffer(RefPtr<Buffer> buffer)
	{
		m_bufferTable.Remove(buffer);
	}

	void VulkanResourceTable::RemoveTexture(RefPtr<Image> texture)
	{
		m_textureTable.Remove(texture);
	}

	void* VulkanResourceTable::GetHandleImpl() const
	{
		return nullptr;
	}

	void VulkanResourceTable::Update(uint32_t index)
	{
		if (index == m_lastUpdateIndex)
		{
			return;
		}

		m_lastUpdateIndex = index;
		m_currentBufferIndex = index % RHI::RHICapabilities::NumFramesInFlight;

		Vector<uint32_t> dirtyBufferSlots = m_bufferTable.GetAndClearDirtySlots(m_currentBufferIndex);
		Vector<uint32_t> dirtyTextureSlots = m_textureTable.GetAndClearDirtySlots(m_currentBufferIndex);

		const ArrayView<uint64_t> bindingOffsets = ResourceTableDescriptorSetManager::Get().GetBindingOffsets();

		auto device = GraphicsContext::GetDevice();
		VkDevice vkDevice = device->GetHandle<VkDevice>();

		uint8_t* bufferBindingDescriptorPtr = GetHeapPointer() + bindingOffsets[ResourceTableDescriptorSetManager::BuffersBinding];
		uint8_t* texturesBindingDescriptorPtr = GetHeapPointer() + bindingOffsets[ResourceTableDescriptorSetManager::TexturesBinding];

		for (size_t i = 0; i < dirtyBufferSlots.size(); ++i)
		{
			const uint32_t slotIndex = dirtyBufferSlots[i];

			RefPtr<BufferView> bufferView = m_bufferTable.GetViewAtSlot(slotIndex);

			VulkanBufferView& vkBufferView = bufferView->AsRef<VulkanBufferView>();
			const VulkanBufferView::DescriptorDescription& srvDescriptor = vkBufferView.GetSRVDescriptor();

			const uint64_t descriptorSize = g_physicalDeviceProperties.descriptorBufferProperties.storageBufferDescriptorSize;
			vkGetDescriptorEXT(vkDevice, &srvDescriptor.vkDescriptorInfo, descriptorSize, bufferBindingDescriptorPtr + descriptorSize * slotIndex);
		}

		for (size_t i = 0; i < dirtyTextureSlots.size(); i++)
		{
			const uint32_t slotIndex = dirtyTextureSlots[i];

			RefPtr<ImageView> imageView = m_textureTable.GetViewAtSlot(slotIndex);

			VulkanImageView& vkImageView = imageView->AsRef<VulkanImageView>();
			const VulkanImageView::DescriptorDescription& srvDescriptor = vkImageView.GetSRVDescriptor();

			const uint64_t descriptorSize = g_physicalDeviceProperties.descriptorBufferProperties.sampledImageDescriptorSize;
			vkGetDescriptorEXT(vkDevice, &srvDescriptor.vkDescriptorInfo, descriptorSize, texturesBindingDescriptorPtr + descriptorSize * slotIndex);
		}
	}

	uint32_t VulkanResourceTable::GetBufferSlotIndex(RefPtr<Buffer> buffer)
	{
		return m_bufferTable.GetSlotForResource(buffer);
	}

	uint32_t VulkanResourceTable::GetTextureSlotIndex(RefPtr<Image> texture)
	{
		return m_textureTable.GetSlotForResource(texture);
	}

	void VulkanResourceTable::Initialize()
	{
		m_descriptorSetLayoutSize = ResourceTableDescriptorSetManager::Get().GetDescriptorSetLayoutSize();

		BufferDesc bufferDesc{};
		bufferDesc.numElements = 1;
		bufferDesc.elementSize = m_descriptorSetLayoutSize * RHICapabilities::NumFramesInFlight;
		bufferDesc.usage = BufferUsage::DescriptorBuffer | BufferUsage::DeviceAddress;
		bufferDesc.memoryUsage = MemoryUsage::CPUToGPU;
		bufferDesc.debugName = "RayTracingDescriptorHeap";

		m_descriptorHeapAllocation = GraphicsContext::Get().GetDefaultAllocator()->CreateBuffer(bufferDesc);
		m_mappedPtr = m_descriptorHeapAllocation->Map<uint8_t>();
	}

	void VulkanResourceTable::Release()
	{
		if (m_mappedPtr)
		{
			m_descriptorHeapAllocation->Unmap();
			m_mappedPtr = nullptr;
		}

		GraphicsContext::Get().GetDefaultAllocator()->DestroyBuffer(m_descriptorHeapAllocation);
	}

	uint64_t VulkanResourceTable::GetBaseOffset() const
	{
		return static_cast<uint64_t>(m_currentBufferIndex) * m_descriptorSetLayoutSize;
	}

	uint8_t* VulkanResourceTable::GetHeapPointer()
	{
		return &m_mappedPtr[GetBaseOffset()];
	}

	uint64_t VulkanResourceTable::GetDeviceAddress() const
	{
		return m_descriptorHeapAllocation->GetDeviceAddress();
	}

	uint32_t VulkanResourceTable::GetOrAddBufferSlotIndex(RefPtr<Buffer> buffer)
	{
		return m_bufferTable.GetOrAddSlotForResource(buffer);
	}

	uint32_t VulkanResourceTable::GetOrAddTextureSlotIndex(RefPtr<Image> texture)
	{
		return m_textureTable.GetOrAddSlotForResource(texture);
	}

}
