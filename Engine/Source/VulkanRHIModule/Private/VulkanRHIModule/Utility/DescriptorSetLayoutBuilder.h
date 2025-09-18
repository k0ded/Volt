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
			// Used only for creation of the pipeline layout, because it requires the
			// the descriptor set indices to be sequential, which might not be true.
			Vector<VkDescriptorSetLayout_T*> pipelineLayoutDescriptorSetLayouts;
			Map<uint32_t, VkDescriptorSetLayout_T*> descriptorSetLayouts;
		};

		DescriptorSets BuildFromShaderResourceBindings(const ShaderParameterMap::ResourceBindings& resourceBindings, bool accessesRayTracingResourceTable);
		DescriptorSets BuildFromShaderResourceBindings(const Vector<ShaderParameterMap::ResourceBindings>& resourceBindings, bool accessesRayTracingResourceTable);
	
		Vector<std::pair<uint32_t, uint32_t>> CalculateDescriptorPoolSizesFromBindings(const ShaderParameterMap::ResourceBindings& resourceBindings);
		Vector<std::pair<uint32_t, uint32_t>> CalculateDescriptorPoolSizesFromBindings(const Vector<ShaderParameterMap::ResourceBindings>& resourceBindings);
	};
}
