#pragma once

#include "VulkanRHIModule/Shader/VulkanShader2.h"

struct VkDescriptorSetLayout_T;

namespace Volt::RHI
{
	class DescriptorSetLayoutBuilder
	{
	public:
		Vector<VkDescriptorSetLayout_T*> BuildFromShaderBindings(const ShaderBindings& bindings);
		Vector<VkDescriptorSetLayout_T*> BuildFromShaderBindings(const Vector<ShaderBindings>& bindings);
	
		Vector<std::pair<uint32_t, uint32_t>> CalculateDescriptorPoolSizesFromBindings(const ShaderBindings& shaderBindings);
		Vector<std::pair<uint32_t, uint32_t>> CalculateDescriptorPoolSizesFromBindings(const Vector<ShaderBindings>& shaderBindings);

		ShaderBindings GetMergedShaderBindings(const Vector<ShaderBindings>& shaderBindings);
	};
}
