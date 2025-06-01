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
			vt::map<uint32_t, VkDescriptorSetLayout_T*> descriptorSetLayouts;
		};

		DescriptorSets BuildFromShaderResourceBindings(const ShaderParameterMap::ResourceBindingsMap& resourceBindings);
		DescriptorSets BuildFromShaderResourceBindings(const Vector<ShaderParameterMap::ResourceBindingsMap>& resourceBindings);
	
		Vector<std::pair<uint32_t, uint32_t>> CalculateDescriptorPoolSizesFromBindings(const ShaderParameterMap::ResourceBindingsMap& resourceBindings);
		Vector<std::pair<uint32_t, uint32_t>> CalculateDescriptorPoolSizesFromBindings(const Vector<ShaderParameterMap::ResourceBindingsMap>& resourceBindings);
	};
}
