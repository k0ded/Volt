#include "vkpch.h"

#include "VulkanRHIModule/Utility/DescriptorSetLayoutBuilder.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"
#include "VulkanRHIModule/Descriptors/ResourceTableDescriptorSetManager.h"
#include "VulkanRHIModule/Descriptors/VulkanBindlessDescriptorManager.h"
#include "VulkanRHIModule/Graphics/PhysicalDeviceProperties.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"
#include "VulkanRHIModule/Pipelines/StaticSamplerDescriptorSetManager.h"
#include "VulkanRHIModule/VulkanResourceCast.h"

#include <RHIModule/Globals.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIFeatures.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/MemoryUtility.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	DescriptorSetLayoutBuilder::DescriptorSets DescriptorSetLayoutBuilder::BuildFromShaderResourceBindings(const ShaderParameterMap::ResourceBindings& resourceBindings, bool accessesResourceTable)
	{
		DescriptorSets result;
		result.accessesResourceTable = accessesResourceTable;

		auto device = GraphicsContext::GetDevice();

		std::map<uint32_t, Vector<VkDescriptorSetLayoutBinding>> descriptorSetBindings;

		for (const auto& [binding, nameHash] : resourceBindings)
		{
			if (binding.isBindless)
			{
				continue;
			}

			auto& descriptorBinding = descriptorSetBindings[binding.set].emplace_back();
			descriptorBinding.binding = binding.binding;
			descriptorBinding.descriptorCount = 1;
			descriptorBinding.pImmutableSamplers = nullptr;
			descriptorBinding.descriptorCount = binding.arraySize;
			descriptorBinding.stageFlags = Utility::VoltToVulkanShaderStage(binding.shaderStage);

			if (binding.resourceType == ShaderResourceType::UniformBuffer)
			{
				descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			}
			else if (binding.resourceType == ShaderResourceType::Sampler)
			{
				descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			}
			else if (binding.resourceType == ShaderResourceType::StructuredBuffer)
			{
				descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}
			else if (binding.resourceType == ShaderResourceType::TexelBuffer)
			{
				if (binding.registerType == ShaderRegisterType::SRV)
				{
					descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
				}
				else
				{
					descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
				}
			}
			else if (binding.resourceType == ShaderResourceType::Texture)
			{
				if (binding.registerType == ShaderRegisterType::SRV)
				{
					descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				}
				else
				{
					descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				}
			}
			else if (binding.resourceType == ShaderResourceType::AccelerationStructure)
			{
				descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			}
		}

		VulkanGraphicsContext* vulkanGraphicsContext = ResourceCast(&GraphicsContext::Get());

		int32_t lastSet = -1;
		for (const auto& [set, bindings] : descriptorSetBindings)
		{
			// Fill all descriptor set "holes" with empty descriptor sets.
			// Required because descriptor sets are required to be bound in order.
			while (static_cast<int32_t>(set) > lastSet + 1)
			{
				result.pipelineLayoutDescriptorSetLayouts.emplace_back(vulkanGraphicsContext->GetEmptyDescriptorSetLayout());
				lastSet++;
			}

			VkDescriptorSetLayoutCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.bindingCount = static_cast<uint32_t>(bindings.size());
			info.pBindings = bindings.data();
			info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			VT_VK_CHECK(vkCreateDescriptorSetLayout(device->GetHandle<VkDevice>(), &info, VT_VULKAN_ALLOCATOR, &result.pipelineLayoutDescriptorSetLayouts.emplace_back()));
			lastSet = set;

			result.descriptorSetLayouts[set] = result.pipelineLayoutDescriptorSetLayouts.back();

			// Get descriptor set layout size, and make sure it's aligned
			vkGetDescriptorSetLayoutSizeEXT(device->GetHandle<VkDevice>(), result.pipelineLayoutDescriptorSetLayouts.back(), &result.descriptorSetLayoutSizes[set]);
			result.descriptorSetLayoutSizes[set] = ::Utility::Align(result.descriptorSetLayoutSizes[set], g_physicalDeviceProperties.descriptorBufferProperties.descriptorBufferOffsetAlignment);
		
			// Get binding offsets
			for (size_t i = 0; i < bindings.size(); ++i)
			{
				const uint32_t bindingIndex = bindings[i].binding;

				DescriptorSets::Binding& binding = result.descriptorSetLayoutBindings[set][bindingIndex];
				vkGetDescriptorSetLayoutBindingOffsetEXT(device->GetHandle<VkDevice>(), result.pipelineLayoutDescriptorSetLayouts.back(), bindingIndex, &binding.offset);
			}
		}

		if (accessesResourceTable)
		{
			result.pipelineLayoutDescriptorSetLayouts.resize(ResourceTableDescriptorSetManager::Set + 1);

			// Fill all null descriptor set layouts with empty layouts.
			for (uint32_t i = 0; i < ResourceTableDescriptorSetManager::Set; ++i)
			{
				if (result.pipelineLayoutDescriptorSetLayouts[i] == nullptr)
				{
					VkDescriptorSetLayoutCreateInfo info{};
					info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					info.pNext = nullptr;
					info.bindingCount = 0;
					info.pBindings = nullptr;
					info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

					VT_VK_CHECK(vkCreateDescriptorSetLayout(device->GetHandle<VkDevice>(), &info, VT_VULKAN_ALLOCATOR, &result.pipelineLayoutDescriptorSetLayouts[i]));
				}
			}

			result.descriptorSetLayouts[ResourceTableDescriptorSetManager::Set] = ResourceTableDescriptorSetManager::Get().GetDescriptorSetLayout();
			result.pipelineLayoutDescriptorSetLayouts[ResourceTableDescriptorSetManager::Set] = ResourceTableDescriptorSetManager::Get().GetDescriptorSetLayout();
		}

		// Static samplers.
		{
			// Add all 'in-between' descriptor set layouts.
			for (size_t i = result.pipelineLayoutDescriptorSetLayouts.size(); i < StaticSamplerDescriptorSetManager::Set; ++i)
			{
				result.pipelineLayoutDescriptorSetLayouts.emplace_back(vulkanGraphicsContext->GetEmptyDescriptorSetLayout());
			}

			result.pipelineLayoutDescriptorSetLayouts.resize(StaticSamplerDescriptorSetManager::Set + 1);
			result.pipelineLayoutDescriptorSetLayouts[StaticSamplerDescriptorSetManager::Set] = StaticSamplerDescriptorSetManager::Get().GetDescriptorSetLayout();
		}

		// Bindless
		if (RHICanUseBindless())
		{
			// Add all 'in-between' descriptor set layouts.
			for (size_t i = result.pipelineLayoutDescriptorSetLayouts.size(); i < Globals::SHADER_BINDLESS_SPACE; ++i)
			{
				result.pipelineLayoutDescriptorSetLayouts.emplace_back(vulkanGraphicsContext->GetEmptyDescriptorSetLayout());
			}

			result.pipelineLayoutDescriptorSetLayouts.resize(Globals::SHADER_BINDLESS_SPACE + 1);
			result.pipelineLayoutDescriptorSetLayouts[Globals::SHADER_BINDLESS_SPACE] = VulkanBindlessDescriptorManager::Get().GetDescriptorSetLayout();
		}

		return result;
	}

	void AppendBindings(ShaderParameterMap::ResourceBindings& outBindings, const ShaderParameterMap::ResourceBindings& shaderBindings)
	{
		// Because multiple shader stages might have bindings with the same name, we need 
		// to check for and handle duplicates. As the names does not matter here, we can replace them
		// with temporary ones.
		uint32_t duplicateIndex = 0;
		for (const auto& [binding, shaderBindingHash] : shaderBindings)
		{
			StringHash newNameHash = shaderBindingHash;

			for (const auto& [newBinding, newBindingHash] : outBindings)
			{
				if (shaderBindingHash == newBindingHash)
				{
					newNameHash = StringHash::Construct(FormatString("Duplicate{}", duplicateIndex++));
				}
			}

			outBindings.emplace_back(binding, newNameHash);
		}
	}

	DescriptorSetLayoutBuilder::DescriptorSets DescriptorSetLayoutBuilder::BuildFromShaderResourceBindings(const Vector<ShaderParameterMap::ResourceBindings>& bindings, bool accessesRayTracingResourceTable)
	{
		// With multiple shaders, we start by merging all resources.
		ShaderParameterMap::ResourceBindings mergedShaderBindings;

		for (const auto& shaderBindings : bindings)
		{
			AppendBindings(mergedShaderBindings, shaderBindings);
		}

		// Now we create descriptor set layouts of the merged bindings.
		return BuildFromShaderResourceBindings(mergedShaderBindings, accessesRayTracingResourceTable);
	}

	Vector<std::pair<uint32_t, uint32_t>> DescriptorSetLayoutBuilder::CalculateDescriptorPoolSizesFromBindings(const ShaderParameterMap::ResourceBindings& resourceBindings)
	{
		uint32_t uboCount = 0;
		uint32_t ssboCount = 0;
		uint32_t uniformTexelBufferCount = 0;
		uint32_t storageTexelBufferCount = 0;
		uint32_t storageImageCount = 0;
		uint32_t imageCount = 0;
		uint32_t seperateSamplerCount = 0;
		uint32_t accelerationStructureCount = 0;

		for (const auto& [binding, nameHash] : resourceBindings)
		{
			if (binding.resourceType == ShaderResourceType::UniformBuffer)
			{
				uboCount += binding.arraySize;
			}
			else if (binding.resourceType == ShaderResourceType::Sampler)
			{
				seperateSamplerCount += binding.arraySize;
			}
			else if (binding.resourceType == ShaderResourceType::StructuredBuffer)
			{
				ssboCount += binding.arraySize;
			}
			else if (binding.resourceType == ShaderResourceType::TexelBuffer)
			{
				if (binding.registerType == ShaderRegisterType::SRV)
				{
					uniformTexelBufferCount += binding.arraySize;
				}
				else
				{
					storageTexelBufferCount += binding.arraySize;
				}
			}
			else if (binding.resourceType == ShaderResourceType::Texture)
			{
				if (binding.registerType == ShaderRegisterType::SRV)
				{
					imageCount += binding.arraySize;
				}
				else
				{
					storageImageCount += binding.arraySize;
				}
			}
			else if (binding.resourceType == ShaderResourceType::AccelerationStructure)
			{
				accelerationStructureCount += binding.arraySize;
			}
		}

		Vector<std::pair<uint32_t, uint32_t>> result;

		if (uboCount > 0)
		{
			result.emplace_back(static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER), uboCount);
		}

		if (ssboCount > 0)
		{
			result.emplace_back(static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER), ssboCount);
		}

		if (uniformTexelBufferCount > 0)
		{
			result.emplace_back(static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER), uniformTexelBufferCount);
		}

		if (storageTexelBufferCount > 0)
		{
			result.emplace_back(static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER), storageTexelBufferCount);
		}

		if (storageImageCount > 0)
		{
			result.emplace_back(static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE), storageImageCount);
		}

		if (imageCount > 0)
		{
			result.emplace_back(static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE), imageCount);
		}

		if (seperateSamplerCount > 0)
		{
			result.emplace_back(static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_SAMPLER), seperateSamplerCount);
		}

		if (accelerationStructureCount > 0)
		{
			result.emplace_back(static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR), accelerationStructureCount);
		}

		return result;
	}
	
	Vector<std::pair<uint32_t, uint32_t>> DescriptorSetLayoutBuilder::CalculateDescriptorPoolSizesFromBindings(const Vector<ShaderParameterMap::ResourceBindings>& shaderBindings)
	{
		// With multiple shaders, we start by merging all resources.
		ShaderParameterMap::ResourceBindings mergedShaderBindings;

		for (const auto& bindings : shaderBindings)
		{
			AppendBindings(mergedShaderBindings, bindings);
		}

		// Now we create descriptor set layouts of the merged bindings.
		return CalculateDescriptorPoolSizesFromBindings(mergedShaderBindings);
	}
}
