#include "vkpch.h"

#include "VulkanRHIModule/Utility/DescriptorSetLayoutBuilder.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"

#include <RHIModule/Globals.h>
#include <RHIModule/Graphics/GraphicsContext.h>

#include <CoreUtilities/Containers/Map.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	DescriptorSetLayoutBuilder::DescriptorSets DescriptorSetLayoutBuilder::BuildFromShaderResourceBindings(const ShaderParameterMap::ResourceBindingsMap& resourceBindings)
	{
		std::map<uint32_t, Vector<VkDescriptorSetLayoutBinding>> descriptorSetBindings;

		for (const auto& [nameHash, binding] : resourceBindings)
		{
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

		DescriptorSets result;

		auto device = GraphicsContext::GetDevice();

		int32_t lastSet = -1;
		for (const auto& [set, bindings] : descriptorSetBindings)
		{
			// Fill all descriptor set "holes" with empty descriptor sets.
			// Required because descriptor sets are required to be bound in order.
			while (static_cast<int32_t>(set) > lastSet + 1)
			{
				VkDescriptorSetLayoutCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				info.pNext = nullptr;
				info.bindingCount = 0;
				info.pBindings = nullptr;
				info.flags = 0;

				VT_VK_CHECK(vkCreateDescriptorSetLayout(device->GetHandle<VkDevice>(), &info, VT_VULKAN_ALLOCATOR, &result.pipelineLayoutDescriptorSetLayouts.emplace_back()));
				lastSet++;
			}

			VkDescriptorSetLayoutCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.bindingCount = static_cast<uint32_t>(bindings.size());
			info.pBindings = bindings.data();
			info.flags = 0;

			VT_VK_CHECK(vkCreateDescriptorSetLayout(device->GetHandle<VkDevice>(), &info, VT_VULKAN_ALLOCATOR, &result.pipelineLayoutDescriptorSetLayouts.emplace_back()));
			lastSet = set;

			result.descriptorSetLayouts[set] = result.pipelineLayoutDescriptorSetLayouts.back();
		}

		return result;
	}

	void AppendBindings(ShaderParameterMap::ResourceBindingsMap& outBindings, const ShaderParameterMap::ResourceBindingsMap& shaderBindings)
	{
		// Because multiple shader stages might have bindings with the same name, we need 
		// to check for and handle duplicates. As the names does not matter here, we can replace them
		// with temporary ones.
		uint32_t duplicateIndex = 0;
		for (const auto& [nameHash, binding] : shaderBindings)
		{
			StringHash newNameHash = nameHash;

			if (outBindings.contains(nameHash))
			{
				newNameHash = StringHash::Construct("Duplicate" + std::to_string(duplicateIndex++));
			}

			outBindings[newNameHash] = binding;
		}
	}

	DescriptorSetLayoutBuilder::DescriptorSets DescriptorSetLayoutBuilder::BuildFromShaderResourceBindings(const Vector<ShaderParameterMap::ResourceBindingsMap>& bindings)
	{
		// With multiple shaders, we start by merging all resources.
		ShaderParameterMap::ResourceBindingsMap mergedShaderBindings;

		for (const auto& shaderBindings : bindings)
		{
			AppendBindings(mergedShaderBindings, shaderBindings);
		}

		// Now we create descriptor set layouts of the merged bindings.
		return BuildFromShaderResourceBindings(mergedShaderBindings);
	}

	Vector<std::pair<uint32_t, uint32_t>> DescriptorSetLayoutBuilder::CalculateDescriptorPoolSizesFromBindings(const ShaderParameterMap::ResourceBindingsMap& resourceBindings)
	{
		uint32_t uboCount = 0;
		uint32_t ssboCount = 0;
		uint32_t uniformTexelBufferCount = 0;
		uint32_t storageTexelBufferCount = 0;
		uint32_t storageImageCount = 0;
		uint32_t imageCount = 0;
		uint32_t seperateSamplerCount = 0;
		uint32_t accelerationStructureCount = 0;

		for (const auto& [nameHash, binding] : resourceBindings)
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
	
	Vector<std::pair<uint32_t, uint32_t>> DescriptorSetLayoutBuilder::CalculateDescriptorPoolSizesFromBindings(const Vector<ShaderParameterMap::ResourceBindingsMap>& shaderBindings)
	{
		// With multiple shaders, we start by merging all resources.
		ShaderParameterMap::ResourceBindingsMap mergedShaderBindings;

		for (const auto& bindings : shaderBindings)
		{
			AppendBindings(mergedShaderBindings, bindings);
		}

		// Now we create descriptor set layouts of the merged bindings.
		return CalculateDescriptorPoolSizesFromBindings(mergedShaderBindings);
	}
}
