#include "vkpch.h"
#include "VulkanRHIModule/RayTracing/RayTracingTableDescriptorSetManager.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"

#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

#include <CoreUtilities/Containers/Array.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	RayTracingTableDescriptorSetManager::RayTracingTableDescriptorSetManager()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;

		CreateDescriptorSetLayout();
	}

	RayTracingTableDescriptorSetManager::~RayTracingTableDescriptorSetManager()
	{
		if (m_descriptorSetLayout)
		{
			vkDestroyDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle<VkDevice>(), m_descriptorSetLayout, VT_VULKAN_ALLOCATOR);
		}

		s_instance = nullptr;
	}

	void RayTracingTableDescriptorSetManager::CreateDescriptorSetLayout()
	{
		constexpr uint32_t DescriptorTypeCount = 2;

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

		constexpr VkDescriptorBindingFlags Flags =
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

		Array<VkDescriptorBindingFlags, DescriptorTypeCount> bindingFlags =
		{
			Flags,
			Flags
		};

		VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
		extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		extendedInfo.pNext = nullptr;
		extendedInfo.bindingCount = DescriptorTypeCount;
		extendedInfo.pBindingFlags = bindingFlags.data();

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = DescriptorTypeCount;
		createInfo.pBindings = descriptorSetLayoutBindings.data();
		createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		createInfo.pNext = &extendedInfo;

		VT_VK_CHECK(vkCreateDescriptorSetLayout(GraphicsContext::GetDevice()->GetHandle<VkDevice>(), &createInfo, VT_VULKAN_ALLOCATOR, &m_descriptorSetLayout));
	}
}
