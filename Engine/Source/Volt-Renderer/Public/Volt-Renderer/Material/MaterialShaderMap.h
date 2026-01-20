#pragma once

#include "Volt-Renderer/Material/MaterialCommon.h"
#include "Volt-Renderer/Material/CompiledMaterialShaders.h"

#include <RHIModule/Shader/Shader.h>

#include <shared_mutex>

namespace Volt
{
	class MaterialShaderMap
	{
	public:
		void Initialize(const std::string& name, CompiledMaterialShaders&& compiledMaterialShaders);

		template<typename T>
		RefPtr<RHI::Shader> GetShader(const T::PermutationVector& permutationVector)
		{
			permutationVector.Validate();
			const size_t permutationHash = permutationVector.GetHash();

			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();

			bool isDefaultShader = false;
			RefPtr<RHI::Shader> shader = GetShaderInternal(typeIndex, permutationHash, isDefaultShader);

			// Try to compile the permutation if we got the default shader.
			if (isDefaultShader)
			{
				if (!m_compiledMaterialShaders.Empty())
				{
					RHI::ShaderPermutationConfig permutationConfig;
					permutationVector.ResolvePermutations(permutationConfig);

					shader = CompileShaderPermutation(typeIndex, permutationHash, std::move(permutationConfig));
				}
			}
			VT_ENSURE(shader);

			return shader;
		}

	private:
		struct ShaderBucket
		{
			Map<size_t, RefPtr<RHI::Shader>> permutations;
			RefPtr<RHI::Shader> defaultShader;
		};

		RefPtr<RHI::Shader> GetShaderInternal(TypeTraits::TypeIndex shaderType, size_t permutationHash, bool& isDefaultShader);
		RefPtr<RHI::Shader> CompileShaderPermutation(TypeTraits::TypeIndex shaderType, size_t permutationHash, RHI::ShaderPermutationConfig&& permutationConfig);

		Map<TypeTraits::TypeIndex, ShaderBucket> m_shaderMap;

		CompiledMaterialShaders m_compiledMaterialShaders;
		std::string m_name;
		std::shared_mutex m_mutex;
	};
}
