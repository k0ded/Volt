#pragma once

#include "RHIModule/Shader/ShaderCompiler.h"

namespace Volt::RHI
{
	struct ShaderCacheCreateInfo
	{
		std::filesystem::path cacheDirectory;
	};

	struct CachedShaderResult
	{
		ShaderCompiler::CompilationResultData2 data;
		uint64_t timeSinceLastCompile = 0;
	};

	class VTRHI_API ShaderCache : public RefCounted<ShaderCache>
	{
	public:
		ShaderCache(const ShaderCacheCreateInfo& cacheInfo);
		~ShaderCache();

		CachedShaderResult TryGetCachedShader(const ShaderCompiler::Specification2& shaderSpecification);
		void CacheShader(const ShaderCompiler::Specification2& shaderSpec, const ShaderCompiler::CompilationResultData2& compilationResult);

	private:
		std::filesystem::path GetCachedFilePath(const ShaderCompiler::Specification2& shaderSpec) const;
		ShaderCacheCreateInfo m_info;
	};
}
