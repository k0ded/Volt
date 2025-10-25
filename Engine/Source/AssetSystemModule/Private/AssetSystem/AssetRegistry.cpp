#include "aspch.h"
#include "AssetSystem/AssetRegistry.h"
#include "AssetSystem/Serialization/AssetSerializationCommon.h"
#include "AssetSystem/Serialization/AssetSerializer.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <JobSystem/TaskGraph.h>

#include <CoreUtilities/FileSystem.h>
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

	AssetRegistry::AssetRegistry(const std::filesystem::path& engineDirectoryPath, const std::filesystem::path& projectDirectoryPath, std::string_view assetsDirectoryName)
		: m_engineDirectoryPath(engineDirectoryPath),
		m_projectDirectoryPath(projectDirectoryPath),
		m_assetsDirectoryName(assetsDirectoryName)
	{
		Initialize();
	}

	AssetMetadata* AssetRegistry::GetAssetMetadata(AssetHandle assetHandle)
	{
		uint64_t metadataIndirectionIndex;
		if (m_hashTable.Get(assetHandle, metadataIndirectionIndex))
		{
			return m_metadataIndirection[metadataIndirectionIndex];
		}

		return nullptr;
	}

	AssetMetadata* AssetRegistry::GetAssetMetadata(AssetHandle assetHandle) const
	{
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
		return m_hashTable.Get(assetHandle, temp);
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
		// #TODO_AssetSystem: Seperate out, so that engine asset meta data loading stalls, but project asset meta data continues in the background.

		VT_LOGC(Info, LogAssetSystem, "Fetching asset meta data...");
		ScopedTimer timer{};

		Vector<std::filesystem::path> foundAssets = ScanForAssets();

		TaskGraph taskGraph{ ExecutionPriority::Immediate };

		for (const std::filesystem::path& assetFilepath : foundAssets)
		{
			taskGraph.AddTask("Deserialize Asset Metadata", [this, &assetFilepath]()
			{
				AssetMetadata assetMetadata{};
				DeserializeAssetMetadata(assetFilepath, assetMetadata);
				if (assetMetadata.IsValid())
				{
					InsertAssetMetadata(std::move(assetMetadata));
				}
			});
		}

		taskGraph.ExecuteAndWait();

		VT_LOGC(Info, LogAssetSystem, "Finished fetching meta data in {} seconds!", timer.GetTime<Time::Seconds>());
	}

	void AssetRegistry::DeserializeAssetMetadata(const std::filesystem::path& assetFilepath, AssetMetadata& outMetadata)
	{
		VT_PROFILE_FUNCTION();

		constexpr size_t assetHeaderSize = SerializedAssetMetadata::HeaderSize;

		outMetadata.handle = Asset::Null();

		BinaryStreamReader streamReader{ assetFilepath, assetHeaderSize };
		if (!streamReader.IsStreamValid())
		{
			VT_LOGC(Error, LogAssetSystem, "Failed to open asset file: {0}!", assetFilepath);
			return;
		}

		uint32_t value = 0;
		bool couldReadValue = streamReader.TryRead(value);
		if (!couldReadValue || value != SerializedAssetMetadata::AssetMagic)
		{
			return;
		}

		streamReader.ResetHead();

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);

		outMetadata.handle = serializedMetadata.handle;
		outMetadata.filePath = GetRelativeAssetFilepath(assetFilepath);
		outMetadata.type = serializedMetadata.type;
		outMetadata.customData = serializedMetadata.customData;
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

	std::filesystem::path AssetRegistry::GetRelativeAssetFilepath(const std::filesystem::path& filepath)
	{
		const std::filesystem::path normalizedFilepath = filepath.lexically_normal();
		const std::filesystem::path assetsDirectoryFilepath = m_projectDirectoryPath / m_assetsDirectoryName;

		const std::string normalizedFilepathString = normalizedFilepath.string();

		if (normalizedFilepathString.find(assetsDirectoryFilepath.string()) != std::string::npos)
		{
			return std::filesystem::proximate(normalizedFilepath, assetsDirectoryFilepath).generic_string();
		}

		if (normalizedFilepathString.find(m_engineDirectoryPath.string()) != std::string::npos)
		{
			return std::filesystem::proximate(normalizedFilepath, m_engineDirectoryPath).generic_string();
		}

		return normalizedFilepath.generic_string();
	}

	Vector<std::filesystem::path> AssetRegistry::ScanForAssets()
	{
		VT_PROFILE_FUNCTION();

		constexpr std::string_view AssetExtension = ".vtasset";

		const Vector<std::filesystem::path> filepathsToScan =
		{
			m_engineDirectoryPath / "Engine",
			m_engineDirectoryPath / "Editor",
			m_projectDirectoryPath / m_assetsDirectoryName
		};

		TaskGraph scanGraph{ ExecutionPriority::Immediate };

		Vector<Vector<std::filesystem::path>> intermediateFilepaths;
		intermediateFilepaths.resize(filepathsToScan.size());

		for (uint32_t index = 0; const std::filesystem::path& filepathToScan : filepathsToScan)
		{
			// If the directory does not exist, we skip.
			if (!FileSystem::Exists(filepathToScan))
			{
				continue;
			}

			scanGraph.AddTask("Scan Assets", [&intermediateFilepaths, &filepathToScan, index]() 
			{
				for (const auto& pathIt : std::filesystem::recursive_directory_iterator(filepathToScan))
				{
					if (pathIt.path().extension() == AssetExtension)
					{
						intermediateFilepaths[index].emplace_back(pathIt.path());
					}
				}
			});

			index++;
		}

		scanGraph.ExecuteAndWait();

		Vector<std::filesystem::path> resultFilepaths;
	
		for (const Vector<std::filesystem::path>& intermediate : intermediateFilepaths)
		{
			resultFilepaths.append(intermediate);
		}

		return resultFilepaths;
	}
}
