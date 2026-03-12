#include "rhipch.h"
#include "RHIModule/Shader/ShaderParameterMap.h"

#include <CoreUtilities/Archive/ArchiveVersionRegistry.h>

namespace Volt::RHI
{
	struct ShaderParameterMapCustomVersion
	{
		enum Type
		{
			BaseVersion = 0,
			AddedInlineParameterBlock,

			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		inline static constexpr VoltGUID guid = "{5B9B2CC7-B9B6-437A-AED7-945F8A9C8F93}"_guid;
	private:
		ShaderParameterMapCustomVersion() = default;
	};
	ArchiveVersionRegistrar g_registerShaderParameterMapCustomVersion(ShaderParameterMapCustomVersion::guid, ShaderParameterMapCustomVersion::LatestVersion, "ShaderParameterMapCustomVersion");

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

	void ShaderParameterMap::AddInlineParameter(const std::string& name, ShaderUniformType uniformType, uint32_t size, uint32_t offset)
	{
		auto& inlineParameter = m_inlineParameterBlock[StringHash::Construct(name)];
		inlineParameter.type = uniformType;
		inlineParameter.size = size;
		inlineParameter.offset = offset;
		inlineParameter.name = name;

		m_inlineParameterBlockSize = std::max(m_inlineParameterBlockSize, offset + size);
	}

	Archive& operator<<(Archive& archive, ShaderParameterMap::ResourceBinding& value)
	{
		archive << value.binding;
		archive << value.hash;
		return archive;
	}

	Archive& operator<<(Archive& archive, ShaderParameterMap& value)
	{
		archive.UseVersion(ShaderParameterMapCustomVersion::guid);

		archive << value.m_shaderParameterSize;
		archive << value.m_shaderStage;
		archive << value.m_resourceBindings;
		archive << value.m_shaderParameters;
		archive << value.m_accessesRayTracingResourceTable;

		if (!archive.IsLoading() || archive.GetVersion(ShaderParameterMapCustomVersion::guid) >= ShaderParameterMapCustomVersion::AddedInlineParameterBlock)
		{
			archive << value.m_inlineParameterBlockSize;
			archive << value.m_inlineParameterBlock;
		}

		return archive;
	}
}
