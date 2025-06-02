#include "vkpch.h"

#include "VulkanRHIModule/Utility/DescriptorSetLayoutBuilder.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

#include <RHIModule/Globals.h>
#include <RHIModule/Graphics/GraphicsContext.h>

#include <CoreUtilities/Containers/Map.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	Vector<VkDescriptorSetLayout_T*> DescriptorSetLayoutBuilder::BuildFromShaderBindings(const ShaderBindings& shaderBindings)
	{
		vt::map<uint32_t, Vector<VkDescriptorSetLayoutBinding>> descriptorSetBindings;

		constexpr uint32_t UnboundedArraySize = 8192;

		for (const auto [set, bindings] : shaderBindings.uniformBuffers)
		{
			for (const auto& [binding, data] : bindings)
			{
				auto& descriptorBinding = descriptorSetBindings[set].emplace_back();
				descriptorBinding.binding = binding;
				descriptorBinding.descriptorCount = 1;
				descriptorBinding.descriptorType = binding == Globals::SHADER_GLOBALS_BINDING ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorBinding.stageFlags = static_cast<VkShaderStageFlags>(data.usageStages);
			}
		}

		for (const auto& [set, bindings] : shaderBindings.storageBuffers)
		{
			for (const auto& [binding, data] : bindings)
			{
				auto& descriptorBinding = descriptorSetBindings[set].emplace_back();
				descriptorBinding.binding = binding;

				if (data.arraySize == -1)
				{
					descriptorBinding.descriptorCount = UnboundedArraySize;
				}
				else
				{
					descriptorBinding.descriptorCount = static_cast<uint32_t>(data.arraySize);
				}

				descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorBinding.stageFlags = static_cast<VkShaderStageFlags>(data.usageStages);
			}
		}

		for (const auto& [set, bindings] : shaderBindings.storageImages)
		{
			for (const auto& [binding, data] : bindings)
			{
				auto& descriptorBinding = descriptorSetBindings[set].emplace_back();
				descriptorBinding.binding = binding;

				if (data.arraySize == -1)
				{
					descriptorBinding.descriptorCount = UnboundedArraySize;
				}
				else
				{
					descriptorBinding.descriptorCount = static_cast<uint32_t>(data.arraySize);
				}

				descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				descriptorBinding.stageFlags = static_cast<VkShaderStageFlags>(data.usageStages);
			}
		}

		for (const auto& [set, bindings] : shaderBindings.images)
		{
			for (const auto& [binding, data] : bindings)
			{
				auto& descriptorBinding = descriptorSetBindings[set].emplace_back();
				descriptorBinding.binding = binding;

				if (data.arraySize == -1)
				{
					descriptorBinding.descriptorCount = UnboundedArraySize;
				}
				else
				{
					descriptorBinding.descriptorCount = static_cast<uint32_t>(data.arraySize);
				}

				descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				descriptorBinding.stageFlags = static_cast<VkShaderStageFlags>(data.usageStages);
			}
		}

		for (const auto& [set, bindings] : shaderBindings.samplers)
		{
			for (const auto& [binding, data] : bindings)
			{
				auto& descriptorBinding = descriptorSetBindings[set].emplace_back();
				descriptorBinding.binding = binding;
				descriptorBinding.descriptorCount = 1;
				descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				descriptorBinding.stageFlags = static_cast<VkShaderStageFlags>(data.usageStages);
			}
		}

		Vector<VkDescriptorSetLayout> descriptorSetLayouts;

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

				VT_VK_CHECK(vkCreateDescriptorSetLayout(device->GetHandle<VkDevice>(), &info, nullptr, &descriptorSetLayouts.emplace_back()));
				lastSet++;
			}

			VkDescriptorSetLayoutCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.bindingCount = static_cast<uint32_t>(bindings.size());
			info.pBindings = bindings.data();
			info.flags = 0;

			VT_VK_CHECK(vkCreateDescriptorSetLayout(device->GetHandle<VkDevice>(), &info, nullptr, &descriptorSetLayouts.emplace_back()));
			lastSet = set;
		}

		return descriptorSetLayouts;
	}

	void AppendBindings(ShaderBindings& outBindings, const ShaderBindings& shaderBindings)
	{
		for (const auto [set, bindings] : shaderBindings.uniformBuffers)
		{
			for (const auto& [binding, data] : bindings)
			{
				outBindings.uniformBuffers[set][binding] = data;
			}
		}

		for (const auto [set, bindings] : shaderBindings.storageBuffers)
		{
			for (const auto& [binding, data] : bindings)
			{
				outBindings.storageBuffers[set][binding] = data;
			}
		}

		for (const auto [set, bindings] : shaderBindings.storageImages)
		{
			for (const auto& [binding, data] : bindings)
			{
				outBindings.storageImages[set][binding] = data;
			}
		}

		for (const auto [set, bindings] : shaderBindings.images)
		{
			for (const auto& [binding, data] : bindings)
			{
				outBindings.images[set][binding] = data;
			}
		}

		for (const auto [set, bindings] : shaderBindings.samplers)
		{
			for (const auto& [binding, data] : bindings)
			{
				outBindings.samplers[set][binding] = data;
			}
		}
	}

	Vector<VkDescriptorSetLayout_T*> DescriptorSetLayoutBuilder::BuildFromShaderBindings(const Vector<ShaderBindings>& bindings)
	{
		// With multiple shaders, we start by merging all resources.
		ShaderBindings mergedShaderBindings;

		for (const auto& shaderBindings : bindings)
		{
			AppendBindings(mergedShaderBindings, shaderBindings);
		}

		// Now we create descriptor set layouts of the merged bindings.
		return BuildFromShaderBindings(mergedShaderBindings);
	}

	Vector<std::pair<uint32_t, uint32_t>> DescriptorSetLayoutBuilder::CalculateDescriptorPoolSizesFromBindings(const ShaderBindings& shaderBindings)
	{
		uint32_t uboCount = 0;
		uint32_t ssboCount = 0;
		uint32_t storageImageCount = 0;
		uint32_t imageCount = 0;
		uint32_t seperateSamplerCount = 0;

		for (const auto& [set, bindings] : shaderBindings.uniformBuffers)
		{
			for (const auto& [binding, info] : bindings)
			{
				uboCount += info.usageCount;
			}
		}

		for (const auto& [set, bindings] : shaderBindings.storageBuffers)
		{
			for (const auto& [binding, info] : bindings)
			{
				ssboCount += info.usageCount;
			}
		}

		for (const auto& [set, bindings] : shaderBindings.storageImages)
		{
			for (const auto& [binding, info] : bindings)
			{
				storageImageCount += info.usageCount;
			}
		}

		for (const auto& [set, bindings] : shaderBindings.images)
		{
			for (const auto& [binding, info] : bindings)
			{
				imageCount += info.usageCount;
			}
		}

		for (const auto& [set, bindings] : shaderBindings.samplers)
		{
			for (const auto& [binding, info] : bindings)
			{
				seperateSamplerCount += info.usageCount;
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

		return result;
	}
	
	Vector<std::pair<uint32_t, uint32_t>> DescriptorSetLayoutBuilder::CalculateDescriptorPoolSizesFromBindings(const Vector<ShaderBindings>& shaderBindings)
	{
		// With multiple shaders, we start by merging all resources.
		ShaderBindings mergedShaderBindings;

		for (const auto& bindings : shaderBindings)
		{
			AppendBindings(mergedShaderBindings, bindings);
		}

		// Now we create descriptor set layouts of the merged bindings.
		return CalculateDescriptorPoolSizesFromBindings(mergedShaderBindings);
	}

	ShaderBindings DescriptorSetLayoutBuilder::GetMergedShaderBindings(const Vector<ShaderBindings>& shaderBindings)
	{
		ShaderBindings mergedShaderBindings;

		for (const auto& bindings : shaderBindings)
		{
			AppendBindings(mergedShaderBindings, bindings);
		}

		return mergedShaderBindings;
	}
}
