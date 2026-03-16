#include "vkpch.h"
#include "VulkanRHIModule/Descriptors/ResourceTableDescriptorSetManager.h"

#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"
#include "VulkanRHIModule/Graphics/PhysicalDeviceProperties.h"

#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include <CoreUtilities/Containers/Array.h>
#include <CoreUtilities/MemoryUtility.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	ResourceTableDescriptorSetManager::ResourceTableDescriptorSetManager()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;

		CreateDescriptorSetLayout();
	}

	ResourceTableDescriptorSetManager::~ResourceTableDescriptorSetManager()
	{
		if (m_descriptorSetLayout)
		{
			vkDestroyDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle<VkDevice>(), m_descriptorSetLayout, VT_VULKAN_ALLOCATOR);
		}

		s_instance = nullptr;
	}

	void ResourceTableDescriptorSetManager::CreateDescriptorSetLayout()
	{
		Array<VkDescriptorType, DescriptorTypeCount> descriptorTypes =
		{
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		};

		Array<VkDescriptorSetLayoutBinding, DescriptorTypeCount> descriptorSetLayoutBindings;

		{
			VkDescriptorSetLayoutBinding& texturesBinding = descriptorSetLayoutBindings[0];
			texturesBinding.binding = TexturesBinding;
			texturesBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			texturesBinding.descriptorCount = std::numeric_limits<uint16_t>::max();
			texturesBinding.pImmutableSamplers = nullptr;
			texturesBinding.stageFlags = VK_SHADER_STAGE_ALL;
		}

		{
			VkDescriptorSetLayoutBinding& buffersBinding = descriptorSetLayoutBindings[1];
			buffersBinding.binding = BuffersBinding;
			buffersBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			buffersBinding.descriptorCount = std::numeric_limits<uint16_t>::max();
			buffersBinding.pImmutableSamplers = nullptr;
			buffersBinding.stageFlags = VK_SHADER_STAGE_ALL;
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
		extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		extendedInfo.pNext = nullptr;
		extendedInfo.bindingCount = 0;
		extendedInfo.pBindingFlags = nullptr;

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = DescriptorTypeCount;
		createInfo.pBindings = descriptorSetLayoutBindings.data();
		createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
		createInfo.pNext = &extendedInfo;

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateDescriptorSetLayout(device->GetHandle<VkDevice>(), &createInfo, VT_VULKAN_ALLOCATOR, &m_descriptorSetLayout));

		// Get descriptor set layout size, and make sure it's aligned
		vkGetDescriptorSetLayoutSizeEXT(device->GetHandle<VkDevice>(), m_descriptorSetLayout, &m_descriptorSetLayoutSize);
		m_descriptorSetLayoutSize = ::Utility::Align(m_descriptorSetLayoutSize, g_physicalDeviceProperties.descriptorBufferProperties.descriptorBufferOffsetAlignment);

		Array<uint32_t, DescriptorTypeCount> bindings =
		{
			TexturesBinding,
			BuffersBinding
		};

		for (size_t i = 0; i < m_bindingOffsets.size(); ++i)
		{
			vkGetDescriptorSetLayoutBindingOffsetEXT(device->GetHandle<VkDevice>(), m_descriptorSetLayout, bindings[i], &m_bindingOffsets[i]);
		}
	}
}
