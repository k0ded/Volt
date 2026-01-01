#include "rhipch.h"

#include "RHIModule/Shader/ShaderCache.h"
#include "RHIModule/Graphics/GraphicsContext.h"
#include "RHIModule/Utility/HashUtility.h"

#include <CoreUtilities/Archive/FileArchive.h>
#include <CoreUtilities/Archive/ArchiveVersionRegistry.h>
#include <CoreUtilities/Time/TimeUtility.h>

VT_DEFINE_LOG_CATEGORY(LogShaderCache);

namespace Volt::RHI
{
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

	struct CachedShaderArchiveVersion
	{
		enum Type
		{
			BaseVersion = 0,

			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		inline static constexpr VoltGUID guid = "{3FFD665E-16E1-4EBD-B5DA-16113C996F2F}"_guid;

	private:
		CachedShaderArchiveVersion() {}
	};
	ArchiveVersionRegistrar g_registerCachedShaderArchiveVersion(CachedShaderArchiveVersion::guid, CachedShaderArchiveVersion::LatestVersion, "CachedShaderArchiveVersion");

	struct CachedShaderHeader
	{
		uint64_t timeSinceLastCompile;

		friend Archive& operator<<(Archive& archive, CachedShaderHeader& value)
		{
			archive << value.timeSinceLastCompile;
			return archive;
		}
	};

	struct SerializedShaderData
	{
		Vector<uint32_t> shaderData;
		ShaderStage stage;

		friend Archive& operator<<(Archive& archive, SerializedShaderData& value)
		{
			archive << value.shaderData;
			archive << value.stage;
			return archive;
		}
	};

	struct CachedShader
	{
		CachedShaderHeader header;
		SerializedShaderData serializedShaderData;

		Vector<PixelFormat> outputFormats;
		BufferLayoutMap vertexLayout;
		BufferLayout instanceLayout;
		ShaderParameterMap shaderParameterMap;
		Vector<std::filesystem::path> includeDependencies;

		friend Archive& operator<<(Archive& archive, CachedShader& value)
		{
			archive.UseVersion(CachedShaderArchiveVersion::guid);

			archive << value.header;
			archive << value.serializedShaderData;
			archive << value.outputFormats;
			archive << value.vertexLayout;
			archive << value.instanceLayout;
			archive << value.shaderParameterMap;
			archive << value.includeDependencies;
			return archive;
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
		const std::filesystem::path cachedPath = GetCachedFilePath(shaderSpecification);
		const uint64_t lastWriteTime = TimeUtility::GetLastWriteTime(shaderSpecification.shaderSourceInfo.sourceEntry.filepath);

		FileReader fileReader;
		if (!fileReader.Open(cachedPath))
		{
			VT_LOGC(Error, LogShaderCache,
				"Failed to open cached shader '{}'\n"
				"		Error: {}",
				cachedPath,
				fileReader.GetError());

			return {};
		}

		CachedShader cachedShader;
		fileReader << cachedShader;

		if (cachedShader.header.timeSinceLastCompile < lastWriteTime)
		{
			return {};
		}

		VT_ENSURE(shaderSpecification.shaderSourceInfo.sourceEntry.shaderStage == cachedShader.serializedShaderData.stage);

		CachedShaderResult result{};
		result.timeSinceLastCompile = cachedShader.header.timeSinceLastCompile;
		result.data.result = ShaderCompiler::CompilationResult::Success;

		ShaderCompiler::CompilationResultData& resultData = result.data;
		resultData.shaderBinary = std::move(cachedShader.serializedShaderData.shaderData);
		resultData.outputFormats = std::move(cachedShader.outputFormats);
		resultData.vertexLayout = std::move(cachedShader.vertexLayout);
		resultData.instanceLayout = std::move(cachedShader.instanceLayout);
		resultData.shaderParameterMap = std::move(cachedShader.shaderParameterMap);
		resultData.includeDependencies = std::move(cachedShader.includeDependencies);

		return result;
	}

	void ShaderCache::CacheShader(const ShaderCompiler::Specification& shaderSpec, const ShaderCompiler::CompilationResultData& compilationResult)
	{
		const std::filesystem::path cachedPath = GetCachedFilePath(shaderSpec);

		FileWriter archive{};
		if (!archive.Open(cachedPath))
		{
			VT_LOGC(Error, LogShaderCache,
				"Failed to cache shader '{}'\n"
				"		Error: {}",
				shaderSpec.shaderSourceInfo.sourceEntry.entryPoint,
				archive.GetError());

			return;
		}

		CachedShader cachedShader;
		cachedShader.header.timeSinceLastCompile = TimeUtility::GetTimeSinceEpoch();
		cachedShader.serializedShaderData = { compilationResult.shaderBinary, shaderSpec.shaderSourceInfo.sourceEntry.shaderStage };
		cachedShader.outputFormats = compilationResult.outputFormats;
		cachedShader.vertexLayout = compilationResult.vertexLayout;
		cachedShader.instanceLayout = compilationResult.instanceLayout;
		cachedShader.shaderParameterMap = compilationResult.shaderParameterMap;
		cachedShader.includeDependencies = compilationResult.includeDependencies;

		archive << cachedShader;
		archive.Close();
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
