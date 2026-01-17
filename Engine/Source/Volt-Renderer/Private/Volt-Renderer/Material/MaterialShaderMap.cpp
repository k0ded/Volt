#include "vrpch.h"

#include "Volt-Renderer/Material/MaterialShaderMap.h"

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	inline uint64_t GetShaderKey(MaterialBlendMode blendMode)
	{
		return std::hash<std::underlying_type_t<MaterialBlendMode>>()(std::to_underlying(blendMode));
	}

	void MaterialShaderMap::Initialize(const std::string& name, const std::filesystem::path& shaderFilepath, RefPtr<RHI::Shader> defaultShader)
	{
		m_shaderMap.clear();

		m_name = name;
		m_shaderFilepath = shaderFilepath;
		m_defaultShader = defaultShader;
	}

	RefPtr<RHI::Shader> MaterialShaderMap::GetShader(MaterialBlendMode blendMode)
	{
		VT_PROFILE_FUNCTION();

		if (m_shaderFilepath.empty())
		{
			return m_defaultShader;
		}

		const uint64_t hashKey = GetShaderKey(blendMode);

		{
			std::shared_lock lock{ m_mutex };
			if (m_shaderMap.contains(hashKey))
			{
				return m_shaderMap.at(hashKey);
			}
		}

		{
			VT_PROFILE_SCOPE("Compile Shader");

			std::unique_lock lock{ m_mutex };
			RHI::ShaderCreateInfo shaderSpecification;
			shaderSpecification.name = m_name;
			shaderSpecification.sourceFilepath = m_shaderFilepath;
			shaderSpecification.forceCompile = true;
			shaderSpecification.entryPoint = "MainPS";
			shaderSpecification.stage = RHI::ShaderStage::Pixel;
			shaderSpecification.failureIsFatal = false;

			SetupShaderPermutations(shaderSpecification.permutationConfig, blendMode);

			RefPtr<RHI::Shader> pixelShader = RHI::Shader::Create(shaderSpecification);
			m_shaderMap[hashKey] = pixelShader;

			if (pixelShader->IsValid())
			{
				return pixelShader;
			}
		}

		return m_defaultShader;
	}

	void MaterialShaderMap::SetupShaderPermutations(RHI::ShaderPermutationConfig& permutationConfig, MaterialBlendMode blendMode) const
	{
		permutationConfig.AddPermutation("MATERIAL_BLEND_MODE", std::to_string(std::to_underlying(blendMode)));
	}
}
