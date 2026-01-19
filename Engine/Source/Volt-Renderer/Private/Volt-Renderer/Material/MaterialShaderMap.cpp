#include "vrpch.h"

#include "Volt-Renderer/Material/MaterialShaderMap.h"

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	inline uint64_t GetShaderKey(MaterialBlendMode blendMode)
	{
		return std::hash<std::underlying_type_t<MaterialBlendMode>>()(std::to_underlying(blendMode));
	}

	void MaterialShaderMap::Initialize(const std::string& name, CompiledMaterialShaders&& compiledShaders, RefPtr<RHI::Shader> defaultShader)
	{
		m_shaderMap.clear();

		m_name = name;
		m_defaultShader = defaultShader;
		m_compiledMaterialShaders = std::move(compiledShaders);
	}

	void MaterialShaderMap::SetupShaderPermutations(RHI::ShaderPermutationConfig& permutationConfig, MaterialBlendMode blendMode) const
	{
		permutationConfig.AddPermutation("MATERIAL_BLEND_MODE", std::to_string(std::to_underlying(blendMode)));
	}

	RefPtr<RHI::Shader> MaterialShaderMap::GetShaderInternal(TypeTraits::TypeIndex shaderType, size_t permutationHash)
	{
		VT_PROFILE_FUNCTION();

		std::shared_lock lock{ m_mutex };
		auto it = m_shaderMap.find(shaderType);

		if (it == m_shaderMap.end())
		{
			return nullptr;
		}

		const ShaderBucket& shaderBucket = it->second;

		auto shaderIt = shaderBucket.permutations.find(permutationHash);
		if (shaderIt == shaderBucket.permutations.end())
		{
			return nullptr;
		}

		return shaderIt->second;
	}

	RefPtr<RHI::Shader> MaterialShaderMap::CompileShaderPermutation(TypeTraits::TypeIndex shaderType, size_t permutationHash, RHI::ShaderPermutationConfig&& permutationConfig)
	{
		std::unique_lock lock{ m_mutex };

		ShaderBucket& shaderBucket = m_shaderMap[shaderType];
		
		RefPtr<RHI::Shader> shader;
		{
			VT_PROFILE_SCOPE("Compile Shader");

			const CompiledMaterialShaders::CompiledMaterialShader& compiledShader = m_compiledMaterialShaders.Get(shaderType);

			RHI::ShaderCreateInfo shaderSpecification;
			shaderSpecification.name = m_name;
			shaderSpecification.forceCompile = true;
			shaderSpecification.entryPoint = compiledShader.entryPoint;
			shaderSpecification.stage = RHI::ShaderStage::Pixel;
			shaderSpecification.failureIsFatal = false;
			shaderSpecification.permutationConfig = std::move(permutationConfig);

			shader = RHI::Shader::CreateWithSource(shaderSpecification, compiledShader.compiledShader);
		}

		shaderBucket.permutations[permutationHash] = shader;
		return shader;
	}
}
