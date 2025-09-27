#include "vkpch.h"

#include "VulkanRHIModule/RayTracing/VulkanRayTracingResourceTable.h"
#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/RayTracing/RayTracingTableDescriptorSetManager.h"

#include <RHIModule/Graphics/Swapchain.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIModule.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanRayTracingResourceTable::VulkanRayTracingResourceTable()
	{
		CreateDescriptorSets();
	}

	VulkanRayTracingResourceTable::~VulkanRayTracingResourceTable()
	{
		if (m_descriptorPool)
		{
			RHIModule::GetInstance().DestroyResource([descriptorPool = m_descriptorPool]()
			{
				auto device = GraphicsContext::GetDevice();
				vkDestroyDescriptorPool(device->GetHandle<VkDevice>(), descriptorPool, VT_VULKAN_ALLOCATOR);
			});
		}
	}

	void VulkanRayTracingResourceTable::AddBuffer(RefPtr<StorageBuffer> buffer)
	{
		m_bufferTable.Add(buffer);
	}

	void VulkanRayTracingResourceTable::AddTexture(RefPtr<Image> texture)
	{
		m_textureTable.Add(texture);
	}

	void VulkanRayTracingResourceTable::RemoveBuffer(RefPtr<StorageBuffer> buffer)
	{
		m_bufferTable.Remove(buffer);
	}

	void VulkanRayTracingResourceTable::RemoveTexture(RefPtr<Image> texture)
	{
		m_textureTable.Remove(texture);
	}

	void* VulkanRayTracingResourceTable::GetHandleImpl() const
	{
		return nullptr;
	}

	void VulkanRayTracingResourceTable::CreateDescriptorSets()
	{
		constexpr uint32_t DescriptorTypeCount = 2;
		constexpr uint32_t MaxDescriptors = std::numeric_limits<uint16_t>::max() * RHI::RHICapabilities::NumFramesInFlight;

		Array<VkDescriptorPoolSize, DescriptorTypeCount> descriptorPoolSizes =
		{
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MaxDescriptors },
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MaxDescriptors }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		poolInfo.maxSets = MaxDescriptors * DescriptorTypeCount;
		poolInfo.poolSizeCount = DescriptorTypeCount;
		poolInfo.pPoolSizes = descriptorPoolSizes.data();

		auto vkDevice = GraphicsContext::GetDevice()->GetHandle<VkDevice>();

		VT_VK_CHECK(vkCreateDescriptorPool(vkDevice, &poolInfo, VT_VULKAN_ALLOCATOR, &m_descriptorPool));

		VkDescriptorSetLayout setLayout = RayTracingTableDescriptorSetManager::Get().GetDescriptorSetLayout();
		
		Array<VkDescriptorSetLayout, RHI::RHICapabilities::NumFramesInFlight> descriptorSetLayouts =
		{
			setLayout, setLayout, setLayout
		};

		m_descriptorSets.resize(RHI::RHICapabilities::NumFramesInFlight);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = RHI::RHICapabilities::NumFramesInFlight;
		allocInfo.pSetLayouts = descriptorSetLayouts.data();
	
		VT_VK_CHECK(vkAllocateDescriptorSets(vkDevice, &allocInfo, m_descriptorSets.data()));
	}

	void VulkanRayTracingResourceTable::Update(uint32_t index)
	{
		if (index == m_lastUpdateIndex)
		{
			return;
		}

		m_lastUpdateIndex = index;
		index = index % RHI::RHICapabilities::NumFramesInFlight;

		Vector<uint32_t> dirtyBufferSlots = m_bufferTable.GetAndClearDirtySlots(index);
		Vector<uint32_t> dirtyTextureSlots = m_textureTable.GetAndClearDirtySlots(index);

		m_bufferDescriptorInfo.clear();
		m_bufferDescriptorInfo.resize(dirtyBufferSlots.size());

		m_imageDescriptorInfo.clear();
		m_imageDescriptorInfo.resize(dirtyTextureSlots.size());

		m_writeDescriptors.clear();
		m_writeDescriptors.resize(dirtyBufferSlots.size() + dirtyTextureSlots.size());

		for (size_t i = 0; i < dirtyBufferSlots.size(); ++i)
		{
			const uint32_t slotIndex = dirtyBufferSlots[i];

			auto& writeDescriptor = m_writeDescriptors.at(i);
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.pNext = nullptr;
			writeDescriptor.dstSet = m_descriptorSets.at(index);
			writeDescriptor.dstBinding = RayTracingTableDescriptorSetManager::BuffersBinding;
			writeDescriptor.dstArrayElement = slotIndex;
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

			RefPtr<StorageBuffer> buffer = m_bufferTable.GetAtSlot(slotIndex);

			auto& bufferInfo = m_bufferDescriptorInfo.at(i);
			bufferInfo.buffer = buffer->GetHandle<VkBuffer>();
			bufferInfo.offset = 0;
			bufferInfo.range = buffer->GetByteSize();

			writeDescriptor.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(&bufferInfo);
		}

		const size_t offset = dirtyBufferSlots.size();

		for (size_t i = 0; i < dirtyTextureSlots.size(); i++)
		{
			const uint32_t slotIndex = dirtyTextureSlots[i];

			auto& writeDescriptor = m_writeDescriptors.at(offset + i);
			writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptor.pNext = nullptr;
			writeDescriptor.dstSet = m_descriptorSets.at(index);
			writeDescriptor.dstBinding = RayTracingTableDescriptorSetManager::BuffersBinding;
			writeDescriptor.dstArrayElement = slotIndex;
			writeDescriptor.descriptorCount = 1;
			writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

			RefPtr<ImageView> imageView = m_textureTable.GetViewAtSlot(slotIndex);

			auto& imageInfo = m_imageDescriptorInfo.at(i);
			imageInfo.imageView = imageView->GetHandle<VkImageView>();
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			writeDescriptor.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(&imageInfo);
		}

		if (!m_writeDescriptors.empty())
		{
			const VkWriteDescriptorSet* writeDescriptors = reinterpret_cast<const VkWriteDescriptorSet*>(m_writeDescriptors.data());

			auto device = GraphicsContext::GetDevice();
			vkUpdateDescriptorSets(device->GetHandle<VkDevice>(), (uint32_t)m_writeDescriptors.size(), writeDescriptors, 0, nullptr);
		}
	}

	uint32_t VulkanRayTracingResourceTable::GetBufferSlotIndex(RefPtr<StorageBuffer> buffer)
	{
		return m_bufferTable.GetSlotForResource(buffer);
	}

	uint32_t VulkanRayTracingResourceTable::GetTextureSlotIndex(RefPtr<Image> texture)
	{
		return m_textureTable.GetSlotForResource(texture);
	}

	VkDescriptorSet_T* VulkanRayTracingResourceTable::GetDescriptorSet() const
	{
		return m_descriptorSets.at(m_lastUpdateIndex % RHI::RHICapabilities::NumFramesInFlight);
	}
}
