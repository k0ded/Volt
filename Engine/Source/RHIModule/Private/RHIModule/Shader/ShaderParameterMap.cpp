#include "rhipch.h"
#include "RHIModule/Shader/ShaderParameterMap.h"

namespace Volt::RHI
{
	void ShaderParameterMap::AddUniformBuffer(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::CBV;
		resourceBinding.resourceType = ShaderResourceType::UniformBuffer;
		resourceBinding.shaderStage = shaderStage;
	}

	void ShaderParameterMap::AddSampler(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::Sampler;
		resourceBinding.resourceType = ShaderResourceType::Sampler;
		resourceBinding.shaderStage = shaderStage;
	}
	
	void ShaderParameterMap::AddBufferUAV(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::UAV;
		resourceBinding.resourceType = ShaderResourceType::Buffer;
		resourceBinding.shaderStage = shaderStage;
	}
	
	void ShaderParameterMap::AddBufferSRV(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::SRV;
		resourceBinding.resourceType = ShaderResourceType::Buffer;
		resourceBinding.shaderStage = shaderStage;
	}
	
	void ShaderParameterMap::AddTextureSRV(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::SRV;
		resourceBinding.resourceType = ShaderResourceType::Texture;
		resourceBinding.shaderStage = shaderStage;
	}
	
	void ShaderParameterMap::AddTextureUAV(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::UAV;
		resourceBinding.resourceType = ShaderResourceType::Texture;
		resourceBinding.shaderStage = shaderStage;
	}

	void ShaderParameterMap::AddParameter(std::string_view name, ShaderUniformType uniformType, uint32_t size, uint32_t offset)
	{
		auto& parameter = m_shaderParameters[StringHash::Construct(name)];
		parameter.type = uniformType;
		parameter.size = size;
		parameter.offset = offset;

		m_shaderParameterSize = std::max(m_shaderParameterSize, offset + size);
	}

	const ShaderUniform* ShaderParameterMap::GetParameterFromName(StringHash name) const
	{
		if (m_shaderParameters.contains(name))
		{
			return &m_shaderParameters.at(name);
		}

		return nullptr;
	}

	const ShaderResourceBinding* ShaderParameterMap::GetResourceBindingFromName(StringHash name) const
	{
		if (m_resourceBindings.contains(name))
		{
			return &m_resourceBindings.at(name);
		}

		return nullptr;
	}
}
