#pragma once

#include "VulkanRHIModule/Shader/VulkanShader.h"

struct VkDescriptorSetLayout_T;

namespace Volt::RHI
{
	class DescriptorSetLayoutBuilder
	{
	public:
		struct DescriptorSets
		{
			struct Binding
			{
				uint64_t offset;
			};

			// Used only for creation of the pipeline layout, because it requires the
			// the descriptor set indices to be sequential, which might not be true.
			Vector<VkDescriptorSetLayout_T*> pipelineLayoutDescriptorSetLayouts;
			Map<uint32_t, VkDescriptorSetLayout_T*> descriptorSetLayouts;

			Map<uint32_t, uint64_t> descriptorSetLayoutSizes;
			Map<uint32_t, Map<uint32_t, Binding>> descriptorSetLayoutBindings;
		};

		DescriptorSets BuildFromShaderResourceBindings(const ShaderParameterMap::ResourceBindings& resourceBindings, bool accessesRayTracingResourceTable);
		DescriptorSets BuildFromShaderResourceBindings(const Vector<ShaderParameterMap::ResourceBindings>& resourceBindings, bool accessesRayTracingResourceTable);
	
		Vector<std::pair<uint32_t, uint32_t>> CalculateDescriptorPoolSizesFromBindings(const ShaderParameterMap::ResourceBindings& resourceBindings);
		Vector<std::pair<uint32_t, uint32_t>> CalculateDescriptorPoolSizesFromBindings(const Vector<ShaderParameterMap::ResourceBindings>& resourceBindings);
	};
}
