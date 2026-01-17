#pragma once

#include "Volt-Renderer/Material/MaterialCommon.h"

#include <RHIModule/Shader/Shader.h>

#include <shared_mutex>

namespace Volt
{
	class MaterialShaderMap
	{
	public:
		void Initialize(const std::string& name, const std::filesystem::path& shaderFilepath, RefPtr<RHI::Shader> defaultShader);

		RefPtr<RHI::Shader> GetShader(MaterialBlendMode blendMode);

	private:
		void SetupShaderPermutations(RHI::ShaderPermutationConfig& permutationConfig, MaterialBlendMode blendMode) const;

		Map<uint64_t, RefPtr<RHI::Shader>> m_shaderMap;
		RefPtr<RHI::Shader> m_defaultShader;

		std::filesystem::path m_shaderFilepath;
		std::string m_name;
		std::shared_mutex m_mutex;
	};
}
