#include "rhipch.h"
#include "RHIModule/Shader/ShaderParameterMap.h"

namespace Volt::RHI
{
	void ShaderParameterMap::AddUniformBuffer(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::CBV;
		resourceBinding.resourceType = ShaderResourceType::UniformBuffer;
		resourceBinding.shaderStage = shaderStage;
		resourceBinding.name = name;
	}

	void ShaderParameterMap::AddSampler(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::Sampler;
		resourceBinding.resourceType = ShaderResourceType::Sampler;
		resourceBinding.shaderStage = shaderStage;
		resourceBinding.name = name;
	}
	
	void ShaderParameterMap::AddStructuredBufferUAV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::UAV;
		resourceBinding.resourceType = ShaderResourceType::StructuredBuffer;
		resourceBinding.shaderStage = shaderStage;
		resourceBinding.name = name;
	}
	
	void ShaderParameterMap::AddStructuredBufferSRV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::SRV;
		resourceBinding.resourceType = ShaderResourceType::StructuredBuffer;
		resourceBinding.shaderStage = shaderStage;
		resourceBinding.name = name;
	}

	void ShaderParameterMap::AddTexelBufferUAV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::UAV;
		resourceBinding.resourceType = ShaderResourceType::TexelBuffer;
		resourceBinding.shaderStage = shaderStage;
		resourceBinding.name = name;
	}

	void ShaderParameterMap::AddTexelBufferSRV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::SRV;
		resourceBinding.resourceType = ShaderResourceType::TexelBuffer;
		resourceBinding.shaderStage = shaderStage;
		resourceBinding.name = name;
	}
	
	void ShaderParameterMap::AddTextureSRV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::SRV;
		resourceBinding.resourceType = ShaderResourceType::Texture;
		resourceBinding.shaderStage = shaderStage;
		resourceBinding.name = name;
	}
	
	void ShaderParameterMap::AddTextureUAV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::UAV;
		resourceBinding.resourceType = ShaderResourceType::Texture;
		resourceBinding.shaderStage = shaderStage;
		resourceBinding.name = name;
	}

	void ShaderParameterMap::AddAccelerationStructure(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage)
	{
		auto& resourceBinding = m_resourceBindings[StringHash::Construct(name)];
		resourceBinding.set = set;
		resourceBinding.binding = binding;
		resourceBinding.registerType = ShaderRegisterType::SRV;
		resourceBinding.resourceType = ShaderResourceType::AccelerationStructure;
		resourceBinding.shaderStage = shaderStage;
		resourceBinding.name = name;
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
		if (m_resourceBindings.contains(name))
		{
			return &m_resourceBindings.at(name);
		}

		return nullptr;
	}

	void ShaderParameterMap::Serialize(BinaryStreamWriter& streamWriter, const ShaderParameterMap& data)
	{
		streamWriter.Write(data.m_shaderParameterSize);
		streamWriter.Write(data.m_shaderStage);
		streamWriter.Write(data.m_resourceBindings);
		streamWriter.Write(data.m_shaderParameters);
		streamWriter.Write(data.m_accessesRayTracingResourceTable);
	}
	
	void ShaderParameterMap::Deserialize(BinaryStreamReader& streamReader, ShaderParameterMap& outData)
	{
		streamReader.Read(outData.m_shaderParameterSize);
		streamReader.Read(outData.m_shaderStage);
		streamReader.Read(outData.m_resourceBindings);
		streamReader.Read(outData.m_shaderParameters);
		streamReader.Read(outData.m_accessesRayTracingResourceTable);
	}
}
