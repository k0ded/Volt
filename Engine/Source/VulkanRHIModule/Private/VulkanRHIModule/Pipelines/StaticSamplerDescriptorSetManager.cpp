#include "vkpch.h"

#include "StaticSamplerDescriptorSetManager.h"

#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"
#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

#include <RHIModule/Images/StaticSamplers.h>

namespace Volt::RHI
{
	VkSampler CreateSamplerFromDeclaration(const StaticSamplerDeclaration& declaration)
	{
		VkSamplerCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.anisotropyEnable = declaration.anisotropyLevel != AnisotropyLevel::None ? true : false;
		info.maxAnisotropy = static_cast<float>(declaration.anisotropyLevel);

		info.magFilter = Utility::VoltToVulkanFilter(declaration.magFilter);
		info.minFilter = Utility::VoltToVulkanFilter(declaration.minFilter);
		info.mipmapMode = Utility::VoltToVulkanMipMapMode(declaration.mipFilter);

		info.addressModeU = Utility::VoltToVulkanWrapMode(declaration.wrapMode);
		info.addressModeV = Utility::VoltToVulkanWrapMode(declaration.wrapMode);
		info.addressModeW = Utility::VoltToVulkanWrapMode(declaration.wrapMode);

		info.mipLodBias = declaration.mipLodBias;
		info.minLod = declaration.minLod;
		info.maxLod = declaration.maxLod;

		info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		info.compareEnable = declaration.compareOperator != CompareOperator::None ? true : false;
		info.compareOp = declaration.compareOperator == CompareOperator::None ? VK_COMPARE_OP_ALWAYS : Utility::VoltToVulkanCompareOp(declaration.compareOperator);

		VkSampler result;

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateSampler(device->GetHandle<VkDevice>(), &info, VT_VULKAN_ALLOCATOR, &result));
	
		return result;
	}

	StaticSamplerDescriptorSetManager::StaticSamplerDescriptorSetManager()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;
	
		CreateSamplerStates();
		CreateDescriptorSetLayout();
	}

	StaticSamplerDescriptorSetManager::~StaticSamplerDescriptorSetManager()
	{
		VkDevice device = GraphicsContext::GetDevice()->GetHandle<VkDevice>();

		if (m_descriptorSetLayout)
		{
			vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, VT_VULKAN_ALLOCATOR);
		}

		for (VkSampler sampler : m_staticSamplers)
		{
			vkDestroySampler(device, sampler, VT_VULKAN_ALLOCATOR);
		}

		s_instance = nullptr;
	}

	void StaticSamplerDescriptorSetManager::CreateDescriptorSetLayout()
	{
		Vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;

		for (uint32_t index = 0; VkSampler& sampler : m_staticSamplers)
		{
			VkDescriptorSetLayoutBinding& newBinding = descriptorSetLayoutBindings.emplace_back();
			newBinding.binding = index++;
			newBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			newBinding.descriptorCount = 1;
			newBinding.pImmutableSamplers = &sampler;
			newBinding.stageFlags = VK_SHADER_STAGE_ALL;
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
		extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		extendedInfo.pNext = nullptr;
		extendedInfo.bindingCount = 0;
		extendedInfo.pBindingFlags = nullptr;

		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
		createInfo.pBindings = descriptorSetLayoutBindings.data();
		createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT | VK_DESCRIPTOR_SET_LAYOUT_CREATE_EMBEDDED_IMMUTABLE_SAMPLERS_BIT_EXT;
		createInfo.pNext = &extendedInfo;

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateDescriptorSetLayout(device->GetHandle<VkDevice>(), &createInfo, VT_VULKAN_ALLOCATOR, &m_descriptorSetLayout));
	}

	void StaticSamplerDescriptorSetManager::CreateSamplerStates()
	{
		m_staticSamplers.emplace_back(CreateSamplerFromDeclaration(g_staticPointSampler));
		m_staticSamplers.emplace_back(CreateSamplerFromDeclaration(g_staticBilinearSampler));
		m_staticSamplers.emplace_back(CreateSamplerFromDeclaration(g_staticTrilinearSampler));
		m_staticSamplers.emplace_back(CreateSamplerFromDeclaration(g_staticAnisotropicSampler));

		m_staticSamplers.emplace_back(CreateSamplerFromDeclaration(g_staticPointSamplerClamp));
		m_staticSamplers.emplace_back(CreateSamplerFromDeclaration(g_staticBilinearSamplerClamp));
		m_staticSamplers.emplace_back(CreateSamplerFromDeclaration(g_staticTrilinearSamplerClamp));
		m_staticSamplers.emplace_back(CreateSamplerFromDeclaration(g_staticAnisotropicSamplerClamp));
	}
}
