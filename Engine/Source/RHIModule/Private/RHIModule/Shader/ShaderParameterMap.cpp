#include "rhipch.h"
#include "RHIModule/Shader/ShaderParameterMap.h"

namespace Volt::RHI
{
	void ShaderParameterMap::AddUniformBuffer(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings.emplace_back();
		resourceBinding.binding.set = set;
		resourceBinding.binding.binding = binding;
		resourceBinding.binding.registerType = ShaderRegisterType::CBV;
		resourceBinding.binding.resourceType = ShaderResourceType::UniformBuffer;
		resourceBinding.binding.shaderStage = shaderStage;
		resourceBinding.binding.name = name;
		resourceBinding.hash = StringHash::Construct(name);
	}

	void ShaderParameterMap::AddSampler(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings.emplace_back();
		resourceBinding.binding.set = set;
		resourceBinding.binding.binding = binding;
		resourceBinding.binding.registerType = ShaderRegisterType::Sampler;
		resourceBinding.binding.resourceType = ShaderResourceType::Sampler;
		resourceBinding.binding.shaderStage = shaderStage;
		resourceBinding.binding.name = name;
		resourceBinding.hash = StringHash::Construct(name);
	}
	
	void ShaderParameterMap::AddStructuredBufferUAV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings.emplace_back();
		resourceBinding.binding.set = set;
		resourceBinding.binding.binding = binding;
		resourceBinding.binding.registerType = ShaderRegisterType::UAV;
		resourceBinding.binding.resourceType = ShaderResourceType::StructuredBuffer;
		resourceBinding.binding.shaderStage = shaderStage;
		resourceBinding.binding.name = name;
		resourceBinding.hash = StringHash::Construct(name);
	}
	
	void ShaderParameterMap::AddStructuredBufferSRV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings.emplace_back();
		resourceBinding.binding.set = set;
		resourceBinding.binding.binding = binding;
		resourceBinding.binding.registerType = ShaderRegisterType::SRV;
		resourceBinding.binding.resourceType = ShaderResourceType::StructuredBuffer;
		resourceBinding.binding.shaderStage = shaderStage;
		resourceBinding.binding.name = name;
		resourceBinding.hash = StringHash::Construct(name);
	}

	void ShaderParameterMap::AddTexelBufferUAV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings.emplace_back();
		resourceBinding.binding.set = set;
		resourceBinding.binding.binding = binding;
		resourceBinding.binding.registerType = ShaderRegisterType::UAV;
		resourceBinding.binding.resourceType = ShaderResourceType::TexelBuffer;
		resourceBinding.binding.shaderStage = shaderStage;
		resourceBinding.binding.name = name;
		resourceBinding.hash = StringHash::Construct(name);
	}

	void ShaderParameterMap::AddTexelBufferSRV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings.emplace_back();
		resourceBinding.binding.set = set;
		resourceBinding.binding.binding = binding;
		resourceBinding.binding.registerType = ShaderRegisterType::SRV;
		resourceBinding.binding.resourceType = ShaderResourceType::TexelBuffer;
		resourceBinding.binding.shaderStage = shaderStage;
		resourceBinding.binding.name = name;
		resourceBinding.hash = StringHash::Construct(name);
	}
	
	void ShaderParameterMap::AddTextureSRV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings.emplace_back();
		resourceBinding.binding.set = set;
		resourceBinding.binding.binding = binding;
		resourceBinding.binding.registerType = ShaderRegisterType::SRV;
		resourceBinding.binding.resourceType = ShaderResourceType::Texture;
		resourceBinding.binding.shaderStage = shaderStage;
		resourceBinding.binding.name = name;
		resourceBinding.hash = StringHash::Construct(name);
	}
	
	void ShaderParameterMap::AddTextureUAV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings.emplace_back();
		resourceBinding.binding.set = set;
		resourceBinding.binding.binding = binding;
		resourceBinding.binding.registerType = ShaderRegisterType::UAV;
		resourceBinding.binding.resourceType = ShaderResourceType::Texture;
		resourceBinding.binding.shaderStage = shaderStage;
		resourceBinding.binding.name = name;
		resourceBinding.hash = StringHash::Construct(name);
	}

	void ShaderParameterMap::AddAccelerationStructure(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings.emplace_back();
		resourceBinding.binding.set = set;
		resourceBinding.binding.binding = binding;
		resourceBinding.binding.registerType = ShaderRegisterType::SRV;
		resourceBinding.binding.resourceType = ShaderResourceType::AccelerationStructure;
		resourceBinding.binding.shaderStage = shaderStage;
		resourceBinding.binding.name = name;
		resourceBinding.hash = StringHash::Construct(name);
	}

	void ShaderParameterMap::AddParameter(const std::string& name, ShaderUniformType uniformType, uint32_t size, uint32_t offset)
	{
		auto& parameter = m_shaderParameters[StringHash::Construct(name)];
		parameter.type = uniformType;
		parameter.size = size;
		parameter.offset = offset;
		parameter.name = name;

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
		for (const ResourceBinding& binding : m_resourceBindings)
		{
			if (binding.hash == name)
			{
				return &binding.binding;
			}
		}
		return nullptr;
	}

	Archive& operator<<(Archive& archive, ShaderParameterMap::ResourceBinding& value)
	{
		archive << value.binding;
		archive << value.hash;
		return archive;
	}

	Archive& operator<<(Archive& archive, ShaderParameterMap& value)
	{
		archive << value.m_shaderParameterSize;
		archive << value.m_shaderStage;
		archive << value.m_resourceBindings;
		archive << value.m_shaderParameters;
		archive << value.m_accessesRayTracingResourceTable;

		return archive;
	}
}
