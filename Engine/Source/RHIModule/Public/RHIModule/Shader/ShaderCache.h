#pragma once

#include "RHIModule/Shader/ShaderCompiler.h"

VT_DECLARE_LOG_CATEGORY(LogShaderCache, LogVerbosity::Trace);

namespace Volt::RHI
{
	struct ShaderCacheCreateInfo
	{
	};

	struct CachedShaderResult
	{
		ShaderCompiler::CompilationResultData data;
		uint64_t timeSinceLastCompile = 0;
	};

	class VTRHI_API ShaderCache : public IntRefCounted<ShaderCache>
	{
	public:
		ShaderCache(const ShaderCacheCreateInfo& cacheInfo);
		~ShaderCache();

		CachedShaderResult TryGetCachedShader(const ShaderCompiler::Specification& shaderSpecification);
		void CacheShader(const ShaderCompiler::Specification& shaderSpec, const ShaderCompiler::CompilationResultData& compilationResult);

	private:
		Filesystem::Path GetCachedFilePath(const ShaderCompiler::Specification& shaderSpec) const;
		ShaderCacheCreateInfo m_info;
	};
}
