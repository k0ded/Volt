#include "aspch.h"
#include "AssetSystem/AssetRegistry.h"
#include "AssetSystem/AssetManager.h"
#include "AssetSystem/Serialization/AssetSerializationCommon.h"
#include "AssetSystem/Serialization/AssetSerializer.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <JobSystem/TaskGraph.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Archive/FileArchive.h>

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

	AssetRegistry::AssetRegistry(const std::filesystem::path& engineDirectoryPath, const std::filesystem::path& projectDirectoryPath, std::string_view assetsDirectoryName)
		: m_engineDirectoryPath(engineDirectoryPath),
		m_projectDirectoryPath(projectDirectoryPath),
		m_assetsDirectoryName(assetsDirectoryName)
	{
		Initialize();
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

		Vector<std::filesystem::path> engineAssetFilepaths;
		Vector<std::filesystem::path> projectAssetFilepaths;

		ScanForAssets(engineAssetFilepaths, projectAssetFilepaths);

		// Project assets
		{
			TaskGraph taskGraph{ ExecutionPriority::Critical, static_cast<uint32_t>(projectAssetFilepaths.size()) };
			
			for (const std::filesystem::path& assetFilepath : projectAssetFilepaths)
			{
				taskGraph.AddTask("Deserialize Project Asset Metadata", [this, assetFilepath]() 
				{
					AssetMetadata assetMetadata{};
					DeserializeAssetMetadata(assetFilepath, assetMetadata);
					if (assetMetadata.IsValid())
					{
						if (s_assetRegistryLogAssetScan.GetValue())
						{
							std::string logMessage = std::format(
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
				});
			}

			taskGraph.Execute();
		}

		// Engine assets
		{
			TaskGraph taskGraph{ ExecutionPriority::Immediate };

			for (const std::filesystem::path& assetFilepath : engineAssetFilepaths)
			{
				taskGraph.AddTask("Deserialize Engine Asset Metadata", [this, &assetFilepath]() 
				{
					AssetMetadata assetMetadata{};
					DeserializeAssetMetadata(assetFilepath, assetMetadata);
					if (assetMetadata.IsValid())
					{
						if (s_assetRegistryLogAssetScan.GetValue())
						{
							std::string logMessage = std::format(
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
				});
			}

			taskGraph.ExecuteAndWait();
		}

		VT_LOGC(Info, LogAssetSystem, "Finished fetching meta data in {} seconds!", timer.GetTime<Time::Seconds>());
	}

	void AssetRegistry::DeserializeAssetMetadata(const std::filesystem::path& assetFilepath, AssetMetadata& outMetadata)
	{
		VT_PROFILE_FUNCTION();

		FileReader fileReader;
		if (!fileReader.Open(assetFilepath))
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

	std::filesystem::path AssetRegistry::GetRelativeAssetFilepath(const std::filesystem::path& filepath) const
	{
		const std::filesystem::path normalizedFilepath = filepath.lexically_normal();
		const std::filesystem::path assetsDirectoryFilepath = m_projectDirectoryPath / m_assetsDirectoryName;

		const std::string normalizedFilepathString = normalizedFilepath.string();

		if (normalizedFilepathString.find(assetsDirectoryFilepath.string()) != std::string::npos)
		{
			return std::filesystem::proximate(normalizedFilepath, m_projectDirectoryPath).generic_string();
		}

		if (normalizedFilepathString.find(m_engineDirectoryPath.string()) != std::string::npos)
		{
			return std::filesystem::proximate(normalizedFilepath, m_engineDirectoryPath).generic_string();
		}

		return normalizedFilepath.generic_string();
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

	void AssetRegistry::ScanForAssets(Vector<std::filesystem::path>& outEngineAssets, Vector<std::filesystem::path>& outProjectAssets)
	{
		VT_PROFILE_FUNCTION();

		constexpr std::string_view AssetExtension = ".vtasset";
		constexpr uint32_t NumEngineFilepaths = 2;

		const Array<std::filesystem::path, NumEngineFilepaths> engineFilepathsToScan =
		{
			m_engineDirectoryPath / "Engine",
			m_engineDirectoryPath / "Editor",
		};

		const std::filesystem::path projectFilepathToScan = m_projectDirectoryPath / m_assetsDirectoryName;

		TaskGraph scanGraph{ ExecutionPriority::Immediate };

		Array<Vector<std::filesystem::path>, NumEngineFilepaths> engineIntermediateFilepaths;

		for (uint32_t index = 0; const std::filesystem::path& filepathToScan : engineFilepathsToScan)
		{
			// If the directory does not exist, we skip.
			if (!FileSystem::Exists(filepathToScan))
			{
				continue;
			}

			scanGraph.AddTask("Scan Engine Assets", [&engineIntermediateFilepaths, &filepathToScan, index]() 
			{
				for (const auto& pathIt : std::filesystem::recursive_directory_iterator(filepathToScan))
				{
					if (pathIt.path().extension() == AssetExtension)
					{
						engineIntermediateFilepaths[index].emplace_back(pathIt.path());
					}
				}
			});

			index++;
		}

		// Make sure the project assets directory exists.
		if (FileSystem::Exists(projectFilepathToScan))
		{
			scanGraph.AddTask("Scan Project Assets", [&outProjectAssets, &projectFilepathToScan]() 
			{
				for (const auto& pathIt : std::filesystem::recursive_directory_iterator(projectFilepathToScan))
				{
					if (pathIt.path().extension() == AssetExtension)
					{
						outProjectAssets.emplace_back(pathIt.path());
					}
				}
			});
		}

		scanGraph.ExecuteAndWait();

		for (const Vector<std::filesystem::path>& intermediate : engineIntermediateFilepaths)
		{
			outEngineAssets.append(intermediate);
		}
	}
}
