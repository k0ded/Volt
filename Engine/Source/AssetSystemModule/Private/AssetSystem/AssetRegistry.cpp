#include "aspch.h"
#include "AssetSystem/AssetRegistry.h"
#include "AssetSystem/AssetManager.h"

#include <CoreUtilities/ConsoleVariableRegistry.h>

#include <FileSystemModule/FileIORequest.h>
#include <FileSystemModule/Filesystem.h>
#include <FileSystemModule/IOThreads/IOThreads.h>
#include <FileSystemModule/Iterators/RecursiveDirectoryIterator.h>

#include <JobSystem/TaskGraph.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Time/ScopedTimer.h>

namespace Volt
{
	static ConsoleVariable<int32_t> s_assetRegistryLogAssetScan(
		"a.AssetRegistry.LogAssetScan",
		0,
		"Whether or not to log the asset scan.");

	static ConsoleVariable<int32_t> s_assetRegistryNumMaxAssets(
		"a.AssetRegistry.NumMaxAssets",
		500'000,
		"The asset registry hash table cannot be resized.\n"
		"This is the max number of assets that can exist.");

	AssetRegistry::AssetRegistry(const Filesystem::Path& engineDirectoryPath, const Filesystem::Path& projectDirectoryPath, StringView assetsDirectoryName)
		: m_engineDirectoryPath(engineDirectoryPath),
		m_projectDirectoryPath(projectDirectoryPath),
		m_assetsDirectoryName(assetsDirectoryName)
	{
		Initialize();
	}

	AssetRegistry::~AssetRegistry()
	{
		Volt::JobSystem::DestroyCounter(m_metadataLoadingCounter);
	}

	AssetMetadata* AssetRegistry::GetAssetMetadata(AssetHandle assetHandle)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		uint64_t metadataIndirectionIndex;
		if (m_hashTable.Get(assetHandle, metadataIndirectionIndex))
		{
			return m_metadataIndirection[metadataIndirectionIndex];
		}

		return nullptr;
	}

	AssetMetadata* AssetRegistry::GetAssetMetadata(AssetHandle assetHandle) const
	{
		VT_ENSURE(assetHandle != Asset::Null());

		uint64_t metadataIndirectionIndex;
		if (m_hashTable.Get(assetHandle, metadataIndirectionIndex))
		{
			return m_metadataIndirection[metadataIndirectionIndex];
		}

		return nullptr;
	}

	bool AssetRegistry::IsValidAssetHandle(AssetHandle assetHandle) const
	{
		uint64_t temp;
		return assetHandle != Asset::Null() && m_hashTable.Get(assetHandle, temp);
	}

	void AssetRegistry::Initialize()
	{
		// Initialization
		m_hashTable.Reserve(s_assetRegistryNumMaxAssets.GetValue());
		m_metadataIndirection.resize_uninitialized(s_assetRegistryNumMaxAssets.GetValue());
		LoadAssetMetadata();
	}

	void AssetRegistry::LoadAssetMetadata()
	{
		VT_LOGC(Info, LogAssetSystem, "Fetching asset meta data...");
		ScopedTimer timer{};

		Vector<Filesystem::Path> engineAssetFilepaths;
		Vector<Filesystem::Path> projectAssetFilepaths;

		ScanForAssets(engineAssetFilepaths, projectAssetFilepaths);

		// Project assets
		{
			TaskGraph taskGraph{ ExecutionPriority::Immediate, static_cast<uint32_t>(projectAssetFilepaths.size()) };
			
			for (const Filesystem::Path& assetFilepath : projectAssetFilepaths)
			{
				taskGraph.AddTask("Deserialize Project Asset Metadata", [this, assetFilepath]() 
				{
					AssetMetadata assetMetadata{};
					DeserializeAssetMetadata(assetFilepath, assetMetadata);
					if (assetMetadata.IsValid())
					{
						if (s_assetRegistryLogAssetScan.GetValue())
						{
							String logMessage = FormatString(
								"AssetMetadata with handle {} added: \n"
								"	- Filepath: {}\n"
								"	- Type: {}\n",
								assetMetadata.handle,
								assetMetadata.filepath,
								assetMetadata.type->GetName()
							);

							VT_LOGC_UNFORMATTED(Trace, LogAssetSystem, logMessage);
						}

						InsertAssetMetadata(std::move(assetMetadata));
					}
				}, FiberStackSize::KB32);
			}

			m_metadataLoadingCounter = taskGraph.ExecuteAndExtractCounter();
		}

		// Engine assets
		{
			TaskGraph taskGraph{ ExecutionPriority::Immediate };

			for (const Filesystem::Path& assetFilepath : engineAssetFilepaths)
			{
				taskGraph.AddTask("Deserialize Engine Asset Metadata", [this, &assetFilepath]() 
				{
					AssetMetadata assetMetadata{};
					assetMetadata.isEngineAsset = true;

					DeserializeAssetMetadata(assetFilepath, assetMetadata);
					if (assetMetadata.IsValid())
					{
						if (s_assetRegistryLogAssetScan.GetValue())
						{
							String logMessage = FormatString(
								"AssetMetadata with handle {} added: \n"
								"	- Filepath: {}\n"
								"	- Type: {}\n",
								assetMetadata.handle,
								assetMetadata.filepath,
								assetMetadata.type->GetName()
							);

							VT_LOGC_UNFORMATTED(Trace, LogAssetSystem, logMessage);
						}

						InsertAssetMetadata(std::move(assetMetadata));
					}
				}, FiberStackSize::KB32);
			}

			taskGraph.ExecuteAndWait();
		}

		VT_LOGC(Info, LogAssetSystem, "Finished fetching meta data in {} seconds!", timer.GetTime<Time::Seconds>());
	}

	void AssetRegistry::DeserializeAssetMetadata(const Filesystem::Path& assetFilepath, AssetMetadata& outMetadata)
	{
		VT_PROFILE_FUNCTION();

		IORequestResult<IORequestReadFile_FileReader> result = IOThreads::SubmitRequest<IORequestReadFile_FileReader>("Deserialize Asset Metadata", assetFilepath);
		FileReader& fileReader = result.GetResult();

		if (result.GetResultCode() == IORequestResultCode::Failure)
		{
			VT_LOGC(Error, LogAssetSystem, "Failed to open asset file: '{}'\nError: {}", assetFilepath, fileReader.GetError());
			return;
		}

		AssetHeaderDeserializationResult assetHeaderResult = AssetRegistry::DeserializeAssetHeader(fileReader, outMetadata, 0, false);

		if (assetHeaderResult == AssetHeaderDeserializationResult::InvalidAssetFile)
		{
			VT_LOGC(Error, LogAssetSystem, "Failed to open asset file: '{}'\nError: Invalid asset file.", assetFilepath);
			return;
		}

		outMetadata.filepath = GetRelativeAssetFilepath(assetFilepath);
	}

	void AssetRegistry::InsertAssetMetadata(AssetMetadata&& assetMetadata)
	{
		uint64_t metadataIndex = UINT64_MAX;
		if (m_hashTable.Insert(assetMetadata.handle, metadataIndex))
		{
			AssetMetadata* allocatedAssetMetadata = m_metadata.Allocate();
			*allocatedAssetMetadata = std::move(assetMetadata);

			m_metadataIndirection[metadataIndex] = allocatedAssetMetadata;
		}
	}

	void AssetRegistry::RemoveAssetMetadata(AssetHandle assetHandle, bool unlockMutex)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		// Remove the metadata from the hash table and get it's indirection index.
		// It should now be safe to release the mutex and remove the references.
		uint64_t metadataIndex = UINT64_MAX;
		if (m_hashTable.GetAndRemove(assetHandle, metadataIndex))
		{
			if (unlockMutex)
			{
				m_metadataIndirection.at(metadataIndex)->m_assetMetadataMutex.unlock();
			}
			m_metadata.Free(m_metadataIndirection.at(metadataIndex));
			m_metadataIndirection[metadataIndex] = nullptr;
		}
		else
		{
			VT_ENSURE_MSG(false, "Trying to remove metadata which is not in hash table!");
		}
	}

	Filesystem::Path AssetRegistry::GetRelativeAssetFilepath(const Filesystem::Path& filepath) const
	{
		Filesystem::Path normalizedFilepath = filepath.LexicallyNormal();
		const Filesystem::Path assetsDirectoryFilepath = m_projectDirectoryPath / m_assetsDirectoryName;

		const String normalizedFilepathString = normalizedFilepath.ToString();

		if (normalizedFilepathString.find(assetsDirectoryFilepath.ToString()) != String::npos)
		{
			return Filesystem::Proximate(normalizedFilepath, m_projectDirectoryPath).MakePreferred();
		}

		if (normalizedFilepathString.find(m_engineDirectoryPath.ToString()) != String::npos)
		{
			return Filesystem::Proximate(normalizedFilepath, m_engineDirectoryPath).MakePreferred();
		}

		return normalizedFilepath.MakePreferred();
	}

	int32_t AssetRegistry::GetNumMaxAssets()
	{
		return s_assetRegistryNumMaxAssets.GetValue();
	}

	AssetRegistry::AssetHeaderDeserializationResult AssetRegistry::DeserializeAssetHeader(Archive& archive, AssetMetadata& outAssetMetadata, uint32_t expectedAssetVersion, bool checkAssetVersion)
	{
		VT_ENSURE(archive.IsLoading());

		uint32_t assetMagic;
		uint32_t assetVersion;

		archive << assetMagic;

		if (assetMagic != AssetManager::AssetFileMagic)
		{
			return AssetHeaderDeserializationResult::InvalidAssetFile;
		}

		archive << assetVersion;

		if (checkAssetVersion && assetVersion != expectedAssetVersion)
		{
			return AssetHeaderDeserializationResult::InvalidVersion;
		}

		archive << outAssetMetadata;

		return AssetHeaderDeserializationResult::Success;
	}

	void AssetRegistry::ScanForAssets(Vector<Filesystem::Path>& outEngineAssets, Vector<Filesystem::Path>& outProjectAssets)
	{
		VT_PROFILE_FUNCTION();

		constexpr StringView AssetExtension = ".vtasset";
		constexpr uint32_t NumEngineFilepaths = 2;

		const Array<Filesystem::Path, NumEngineFilepaths> engineFilepathsToScan =
		{
			m_engineDirectoryPath / "Engine",
			m_engineDirectoryPath / "Editor",
		};

		const Filesystem::Path projectFilepathToScan = m_projectDirectoryPath / m_assetsDirectoryName;

		TaskGraph scanGraph{ ExecutionPriority::Immediate };

		Array<Vector<Filesystem::Path>, NumEngineFilepaths> engineIntermediateFilepaths;

		for (uint32_t index = 0; const Filesystem::Path& filepathToScan : engineFilepathsToScan)
		{
			// If the directory does not exist, we skip.
			if (!Filesystem::Exists(filepathToScan))
			{
				continue;
			}

			scanGraph.AddTask("Scan Engine Assets", [&engineIntermediateFilepaths, &filepathToScan, index]() 
			{
				for (const auto& entry : Filesystem::RecursiveDirectoryIterator(filepathToScan))
				{
					if (entry.path.Extension() == AssetExtension)
					{
						engineIntermediateFilepaths[index].emplace_back(entry.path);
					}
				}
			});

			index++;
		}

		// Make sure the project assets directory exists.
		if (Filesystem::Exists(projectFilepathToScan))
		{
			scanGraph.AddTask("Scan Project Assets", [&outProjectAssets, &projectFilepathToScan]() 
			{
				for (const auto& entry : Filesystem::RecursiveDirectoryIterator(projectFilepathToScan))
				{
					if (entry.path.Extension() == AssetExtension)
					{
						outProjectAssets.emplace_back(entry.path);
					}
				}
			});
		}

		scanGraph.ExecuteAndWait();

		for (const Vector<Filesystem::Path>& intermediate : engineIntermediateFilepaths)
		{
			outEngineAssets.append(intermediate);
		}
	}
}
