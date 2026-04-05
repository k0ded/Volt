#include "vrpch.h"

#include "Volt-Renderer/Material/MaterialShaderMap.h"
#include "Volt-Renderer/Material/MaterialShaderRegistry.h"

#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/IORequestCompileShader.h>

#include <FileSystemModule/IOThreads/IOThreads.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	inline uint64_t GetShaderKey(MaterialBlendMode blendMode)
	{
		return std::hash<std::underlying_type_t<MaterialBlendMode>>()(std::to_underlying(blendMode));
	}

	void MaterialShaderMap::Initialize(const String& name, CompiledMaterialShaders&& compiledShaders)
	{
		m_shaderMap.clear();

		m_name = name;
		m_compiledMaterialShaders = std::move(compiledShaders);
	}

	IntRef<RHI::Shader> MaterialShaderMap::GetShaderInternal(TypeTraits::TypeIndex shaderType, size_t permutationIndex, bool& isDefaultShader)
	{
		VT_PROFILE_FUNCTION();

		// #Mutex_Replace
		std::shared_lock lock{ m_mutex };
		auto it = m_shaderMap.find(shaderType);

		if (it == m_shaderMap.end())
		{
			isDefaultShader = true;
			const MaterialShaderRegistry::ShaderRegistrationInfo& registrationInfo = MaterialShaderRegistry::Get().GetShaderRegistrationInfoForShader(shaderType);
			return ShaderMap::Get(registrationInfo.defaultShaderClass);
		}

		const ShaderBucket& shaderBucket = it->second;

		auto shaderIt = shaderBucket.permutations.find(permutationIndex);
		if (shaderIt == shaderBucket.permutations.end())
		{
			isDefaultShader = true;
			return shaderBucket.defaultShader;
		}

		isDefaultShader = false;
		return shaderIt->second;
	}

	IntRef<RHI::Shader> MaterialShaderMap::CompileShaderPermutation(TypeTraits::TypeIndex shaderType, size_t permutationIndex, RHI::ShaderPermutationConfig&& permutationConfig)
	{
		// #Mutex_Replace
		std::unique_lock lock{ m_mutex };

		ShaderBucket& shaderBucket = m_shaderMap[shaderType];
		if (!shaderBucket.defaultShader)
		{
			const MaterialShaderRegistry::ShaderRegistrationInfo& registrationInfo = MaterialShaderRegistry::Get().GetShaderRegistrationInfoForShader(shaderType);
			shaderBucket.defaultShader = ShaderMap::Get(registrationInfo.defaultShaderClass);
		}

		IntRef<RHI::Shader> shader;
		{
			const CompiledMaterialShaders::CompiledMaterialShader& compiledShader = m_compiledMaterialShaders.Get(shaderType);

			RHI::ShaderCreateInfo shaderSpecification;
			shaderSpecification.name = m_name;
			shaderSpecification.forceCompile = true;
			shaderSpecification.entryPoint = compiledShader.entryPoint;
			shaderSpecification.stage = RHI::ShaderStage::Pixel;
			shaderSpecification.failureIsFatal = false;
			shaderSpecification.permutationConfig = std::move(permutationConfig);

			IORequestResult<IORequestCompileShader> result = IOThreads::SubmitRequest<IORequestCompileShader>("Compile Material Shader", shaderSpecification, compiledShader.compiledShader);

			VT_ASSERT(result.GetResultCode() == IORequestResultCode::Success);
			shader = result.GetResult();
		}

		shaderBucket.permutations[permutationIndex] = shader;
		return shader;
	}
}
