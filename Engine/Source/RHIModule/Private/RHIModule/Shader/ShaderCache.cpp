#include "rhipch.h"

#include "RHIModule/Shader/ShaderCache.h"
#include "RHIModule/Graphics/GraphicsContext.h"
#include "RHIModule/Utility/HashUtility.h"
#include "RHIModule/RHIFeatures.h"

#include <CoreModule/Project/ProjectManager.h>

#include <FileSystemModule/FileArchive.h>

#include <FileSystemModule/FileIORequest.h>
#include <FileSystemModule/IOThreads/IOThreads.h>
#include <FileSystemModule/Filesystem.h>

#include <CoreUtilities/Archive/ArchiveVersionRegistry.h>
#include <CoreUtilities/Time/TimeUtility.h>

VT_DEFINE_LOG_CATEGORY(LogShaderCache);

namespace Volt::RHI
{
	constexpr uint32_t ShaderCacheVersion = 1;

	namespace Utility
	{
		inline static Filesystem::Path GetShaderCacheSubDirectory()
		{
			const auto api = GraphicsContext::GetAPI();
			Filesystem::Path subDir;

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
			AddedInternalShaderCacheVersion,

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
		uint32_t shaderCacheVersion;

		friend Archive& operator<<(Archive& archive, CachedShaderHeader& value)
		{
			archive.UseVersion(CachedShaderArchiveVersion::guid);

			archive << value.timeSinceLastCompile;

			if (!archive.IsLoading() || archive.GetVersion(CachedShaderArchiveVersion::guid) >= CachedShaderArchiveVersion::AddedInternalShaderCacheVersion)
			{
				archive << value.shaderCacheVersion;
			}
			else
			{
				value.shaderCacheVersion = 0;
			}

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
		Vector<Filesystem::Path> includeDependencies;

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
		VT_UNUSED(m_info);
	}

	ShaderCache::~ShaderCache()
	{
	}

	CachedShaderResult ShaderCache::TryGetCachedShader(const ShaderCompiler::Specification& shaderSpecification)
	{
		VT_PROFILE_FUNCTION();

		const Filesystem::Path cachedPath = GetCachedFilePath(shaderSpecification);
	
		if (shaderSpecification.shaderSourceInfo.sourceEntry.filepath.IsEmpty() || Filesystem::Exists(cachedPath) == false)
		{
			return {};
		}
		
		const uint64_t lastWriteTime = Filesystem::GetLastWriteTime(shaderSpecification.shaderSourceInfo.sourceEntry.filepath);

		IORequestResult<IORequestReadFile_FileReader> ioResult = IOThreads::SubmitRequest<IORequestReadFile_FileReader>("Read Cached Shader", cachedPath);
		FileReader& fileReader = ioResult.GetResult();

		if (ioResult.GetResultCode() == IORequestResultCode::Failure)
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

		if (cachedShader.header.timeSinceLastCompile < lastWriteTime ||
			cachedShader.header.shaderCacheVersion < ShaderCacheVersion)
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
		const Filesystem::Path cachedPath = GetCachedFilePath(shaderSpec);

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
		cachedShader.header.shaderCacheVersion = ShaderCacheVersion;
		cachedShader.serializedShaderData = { compilationResult.shaderBinary, shaderSpec.shaderSourceInfo.sourceEntry.shaderStage };
		cachedShader.outputFormats = compilationResult.outputFormats;
		cachedShader.vertexLayout = compilationResult.vertexLayout;
		cachedShader.instanceLayout = compilationResult.instanceLayout;
		cachedShader.shaderParameterMap = compilationResult.shaderParameterMap;
		cachedShader.includeDependencies = compilationResult.includeDependencies;

		archive << cachedShader;

		IOThreads::SubmitRequest<IORequestWriteFile_FileWriter>("Write Cached Shader", std::move(archive));
	}

	Filesystem::Path ShaderCache::GetCachedFilePath(const ShaderCompiler::Specification& shaderSpec) const
	{
		const size_t hash = Math::HashCombine(std::hash<Filesystem::Path>()(shaderSpec.shaderSourceInfo.sourceEntry.filepath), std::hash<String>()(shaderSpec.shaderSourceInfo.sourceEntry.entryPoint));
		const auto cacheDir = ProjectManager::GetGeneratedDirectory() / "ShaderCache" / Utility::GetShaderCacheSubDirectory();
		
		WString filename;

		if (RHICanUseBindless())
		{
			filename = FormatString(
				L"{}_{}_{}_{}_{}.vtscache",
				shaderSpec.shaderSourceInfo.sourceEntry.filepath.Stem(),
				shaderSpec.shaderSourceInfo.sourceEntry.entryPoint,
				hash,
				L"Bindless",
				shaderSpec.permutationConfig.GetPermutationIndex());
		}
		else
		{
			filename = FormatString(
				L"{}_{}_{}_{}.vtscache",
				shaderSpec.shaderSourceInfo.sourceEntry.filepath.Stem(),
				shaderSpec.shaderSourceInfo.sourceEntry.entryPoint,
				hash,
				shaderSpec.permutationConfig.GetPermutationIndex());
		}

		const auto cachePath = cacheDir / filename;

		if (!Filesystem::Exists(cacheDir))
		{
			Filesystem::CreateDirectories(cacheDir);
		}

		return cachePath;
	}
}
