#include "rhipch.h"

#include "RHIModule/Shader/ShaderCache.h"
#include "RHIModule/Graphics/GraphicsContext.h"
#include "RHIModule/Utility/HashUtility.h"

#include <CoreUtilities/FileIO/BinaryStreamWriter.h>
#include <CoreUtilities/FileIO/BinaryStreamReader.h>
#include <CoreUtilities/Time/TimeUtility.h>

namespace Volt::RHI
{
	constexpr uint32_t SHADER_CACHE_VERSION = 2; // Increase this when updating the shader cache format!

	namespace Utility
	{
		inline static std::filesystem::path GetShaderCacheSubDirectory()
		{
			const auto api = GraphicsContext::GetAPI();
			std::filesystem::path subDir;

			switch (api)
			{
				case GraphicsAPI::Vulkan: subDir = "Vulkan"; break;
				case GraphicsAPI::D3D12: subDir = "D3D12"; break;
				case GraphicsAPI::MoltenVk: subDir = "MoltenVK"; break;
				case GraphicsAPI::Mock: subDir = "Mock"; break;
			}

			return { subDir };
		}
	}

	struct CachedShaderHeader
	{
		uint64_t timeSinceLastCompile;
	};

	struct SerializedShaderData
	{
		Vector<uint32_t> shaderData;
		ShaderStage stage;

		static void Serialize(BinaryStreamWriter& streamWriter, const SerializedShaderData& data)
		{
			streamWriter.Write(data.stage);
			streamWriter.Write(data.shaderData);
		}

		static void Deserialize(BinaryStreamReader& streamReader, SerializedShaderData& outData)
		{
			streamReader.Read(outData.stage);
			streamReader.Read(outData.shaderData);
		}
	};

	ShaderCache::ShaderCache(const ShaderCacheCreateInfo& cacheInfo)
		: m_info(cacheInfo)
	{
	}

	ShaderCache::~ShaderCache()
	{
	}

	CachedShaderResult ShaderCache::TryGetCachedShader(const ShaderCompiler::Specification& shaderSpecification)
	{
		uint64_t lastWriteTime = TimeUtility::GetLastWriteTime(shaderSpecification.shaderSourceInfo.sourceEntry.filepath);

		BinaryStreamReader streamReader{ GetCachedFilePath(shaderSpecification) };
		if (!streamReader.IsStreamValid())
		{
			return {};
		}

		uint32_t shaderCacheVersion = 0;
		streamReader.Read(shaderCacheVersion);

		if (shaderCacheVersion != SHADER_CACHE_VERSION)
		{
			return {};
		}

		CachedShaderHeader cachedHeader{};
		streamReader.Read(cachedHeader);

		if (cachedHeader.timeSinceLastCompile < lastWriteTime)
		{
			return {}; 
		}

		SerializedShaderData serializedShaderData;
		streamReader.Read(serializedShaderData);

		CachedShaderResult result{};
		result.timeSinceLastCompile = cachedHeader.timeSinceLastCompile;
		result.data.result = ShaderCompiler::CompilationResult::Success;

		ShaderCompiler::CompilationResultData& resultData = result.data;
		resultData.shaderBinary = serializedShaderData.shaderData;

		VT_ENSURE(shaderSpecification.shaderSourceInfo.sourceEntry.shaderStage == serializedShaderData.stage);

		streamReader.Read(resultData.outputFormats);
		
		streamReader.Read(resultData.vertexLayout);
		streamReader.Read(resultData.instanceLayout);
		
		streamReader.Read(resultData.shaderParameterMap);

		return result;
	}

	void ShaderCache::CacheShader(const ShaderCompiler::Specification& shaderSpec, const ShaderCompiler::CompilationResultData& compilationResult)
	{
		BinaryStreamWriter streamWriter{};

		CachedShaderHeader cachedShaderHeader{};
		cachedShaderHeader.timeSinceLastCompile = TimeUtility::GetTimeSinceEpoch();

		streamWriter.Write(SHADER_CACHE_VERSION);
		streamWriter.Write(cachedShaderHeader);

		const SerializedShaderData serializedShaderData = { compilationResult.shaderBinary, shaderSpec.shaderSourceInfo.sourceEntry.shaderStage };

		streamWriter.Write(serializedShaderData);

		// Pixel shader
		streamWriter.Write(compilationResult.outputFormats);

		// Vertex shader
		streamWriter.Write(compilationResult.vertexLayout);
		streamWriter.Write(compilationResult.instanceLayout);

		// Common
		streamWriter.Write(compilationResult.shaderParameterMap);

		streamWriter.WriteToDisk(GetCachedFilePath(shaderSpec), false, 0);
	}

	std::filesystem::path ShaderCache::GetCachedFilePath(const ShaderCompiler::Specification& shaderSpec) const
	{
		const size_t hash = Math::HashCombine(std::hash<std::filesystem::path>()(shaderSpec.shaderSourceInfo.sourceEntry.filepath), std::hash<std::string>()(shaderSpec.shaderSourceInfo.sourceEntry.entryPoint));

		const auto cacheDir = m_info.cacheDirectory / Utility::GetShaderCacheSubDirectory();
		const auto cachePath = cacheDir / (shaderSpec.shaderSourceInfo.sourceEntry.filepath.stem().string() + "_" + shaderSpec.shaderSourceInfo.sourceEntry.entryPoint + "_" + std::to_string(hash) + ".vtshcache");

		if (!std::filesystem::exists(cacheDir))
		{
			std::filesystem::create_directories(cacheDir);
		}

		return cachePath;
	}
}
