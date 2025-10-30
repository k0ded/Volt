#include "aspch.h"
#include "AssetManager_New.h"

#include "AssetSystem/AssetLocks.h"

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/StringUtility.h>

Scope<Volt::AssetManager_New> g_assetManager;

namespace Volt
{
	AssetManager_New::AssetManager_New(const std::filesystem::path& engineDirectoryPath, const std::filesystem::path& projectDirectoryPath, std::string_view assetsDirectoryName)
		: m_assetRegistry(engineDirectoryPath, projectDirectoryPath, assetsDirectoryName)
	{
		m_root.engineDirectoryPath = engineDirectoryPath;
		m_root.projectDirectoryPath = projectDirectoryPath;
		m_root.assetsDirectoryName = assetsDirectoryName;

		CreateDependencyGraphAndAddAssetsFromRegistry();
	}

	AssetManager_New::~AssetManager_New()
	{}

	WriteableAssetMetadata AssetManager_New::GetWriteableAssetMetadata(AssetHandle assetHandle) const
	{
		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		return { assetMetadata};
	}

	ReadOnlyAssetMetadata AssetManager_New::GetReadOnlyAssetMetadata(AssetHandle assetHandle) const
	{
		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		return { assetMetadata };
	}

	AssetMetadata AssetManager_New::GetAssetMetadataCopy(AssetHandle assetHandle) const
	{
		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		if (assetMetadata)
		{
			return *assetMetadata;
		}

		return {};
	}
	
	void AssetManager_New::ReloadAsset(AssetHandle assetHandle)
	{
		RefPtr<Asset_New> asset = m_assetCache.GetAsset(assetHandle);
		ScopedAssetLock assetLock(asset);

		
	}

	void AssetManager_New::SaveAsset(AssetHandle assetHandle)
	{
		RefPtr<Asset_New> asset;
		if (m_assetCache.TryGetAsset(assetHandle, asset))
		{
			SaveAsset(AssetReference<Asset_New>(asset));
		}
		else
		{
			VT_LOGC(Warning, LogAssetSystem, "Tried to save asset with handle {}, but it is not loaded.", assetHandle);
		}
	}

	void AssetManager_New::SaveAsset(AssetReference<Asset_New> asset)
	{
		ScopedAssetReferenceLock lock{ asset };

		if (!AssetSerializerRegistry::Get().HasSerializer(asset->GetType()))
		{
			VT_LOGC(Warning, LogAssetSystem, "No serializer for asset '{}' (Handle: {}) with type {} was found!", asset->GetAssetName(), asset->GetAssetHandle(), asset->GetType()->GetName());
			return;
		}

		if (!asset->IsValid())
		{
			VT_LOGC(Error, LogAssetSystem, "Unable to save invalid asset '{0}' (Handle: '{1}')!", asset->GetAssetName(), asset->GetAssetHandle());
			return;
		}

		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(asset->GetAssetHandle());

		if (assetMetadata->isMemoryAsset)
		{
			VT_LOGC(Error, LogAssetSystem, "Tried to save an asset '{0}' (Handle: '{1}') that is a memory asset. ", asset->GetAssetName(), asset->GetAssetHandle());
			return;
		}

		if (assetMetadata->filepath.empty())
		{
			VT_LOGC(Error, LogAssetSystem, "Tried to save an asset '{0}' (Handle: '{1}') that that does not have a path. ", asset->GetAssetHandle(), asset->GetAssetHandle());
			return;
		}

		{
			ScopedTimer timer{};
		
			// #TODO_AssetSystem: Resolve this situation.
			AssetSerializerRegistry::Get().GetSerializer(assetMetadata->type).Serialize(assetMetadata, const_cast<CustomAssetMetadataVector&>(assetMetadata->customData), asset);

			VT_LOGC(Trace, LogAssetSystem, "Saved asset {0} to {1} in {2} seconds!", assetMetadata->handle, assetMetadata->filepath, timer.GetTime<Time::Seconds>());
		}
	}

	void AssetManager_New::RemoveAsset(AssetHandle assetHandle)
	{
		VT_ENSURE(m_assetRegistry.IsValidAssetHandle(assetHandle));

		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);

		// Lock metadata mutex here.
		assetMetadata->m_assetMetadataMutex.lock();

		m_assetCache.RemoveAsset(assetHandle);
		m_assetRegistry.RemoveAssetMetadata(assetHandle, true);

		m_dependencyGraph->OnAssetChanged(assetHandle, AssetChangedState::Deleted);
		QueueAssetChanged(assetHandle, AssetChangedState::Deleted);

		m_dependencyGraph->RemoveAssetFromGraph(assetHandle);
	}

	bool AssetManager_New::IsValidAssetHandle(AssetHandle assetHandle) const
	{
		return m_assetRegistry.IsValidAssetHandle(assetHandle);
	}

	bool AssetManager_New::IsAssetLoaded(AssetHandle assetHandle) const
	{
		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(assetHandle);
		return assetMetadata->isLoaded;
	}

	bool AssetManager_New::TryGetAssetIfLoadedAsAnonymous(AssetHandle assetHandle, AssetReference<Asset_New>& outAsset)
	{
		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(assetHandle);
		if (assetMetadata->isLoaded)
		{
			// Try to get the asset from the asset cache.
			RefPtr<Asset_New> tempAsset;
			if (m_assetCache.TryGetAsset(assetHandle, tempAsset))
			{
				outAsset = { tempAsset };
				return true;
			}
		}

		return false;
	}

	void AssetManager_New::CreateFileForAsset(AssetHandle assetHandle, const std::filesystem::path& filepath)
	{
		if (FileSystem::FilePathIsOnlyExtension(filepath) || filepath.stem().empty())
		{
			VT_LOGC(Error, LogAssetSystem, "No filename was provided while trying to save asset '{0}'. Target file path: '{1}'", assetHandle, filepath.string().c_str());
			return;
		}

		{
			WriteableAssetMetadata assetMetadata = GetWriteableAssetMetadata(assetHandle);
			if (!assetMetadata.IsValid())
			{
				VT_LOGC(Error, LogAssetSystem, "Tried to create a file for an asset '{0}' that is not registered in the asset registry. Target file path: '{1}'", assetHandle, filepath.string().c_str());
				return;
			}

			if (assetMetadata->isMemoryAsset)
			{
				VT_LOGC(Error, LogAssetSystem, "Tried to create a file for an asset '{0}' that is marked as a memory asset. Target file path: '{1}'", assetHandle, filepath.string().c_str());
				return;
			}

			if (!assetMetadata->filepath.empty())
			{
				VT_LOGC(Warning, LogAssetSystem, "Tried to create a file for an asset '{0}' that already has an assigned file path, overriding!. Target file path: '{1}'", assetHandle, filepath.string().c_str());
			}
	
			assetMetadata->filepath = filepath;
		}

		SaveAsset(assetHandle);
	}

	AssetManager_New::AssetUpdatedCallbackID AssetManager_New::RegisterAssetUpdatedCallback(AssetType assetType, AssetChangedCallback&& callback)
	{
		return {};
	}

	void AssetManager_New::UnregisterAssetUpdatedCallback(AssetType assetType, UUID64 callbackId)
	{

	}

	void AssetManager_New::AddDependencyToAsset(AssetHandle dependant, AssetHandle dependency)
	{
		m_dependencyGraph->AddDependencyToAsset(dependant, dependency);
	}

	Vector<AssetHandle> AssetManager_New::GetAssetsDependentOn(AssetHandle assetHandle) const
	{
		return {};
	}

	void AssetManager_New::QueueAssetChanged(AssetHandle assetHandle, AssetChangedState state)
	{
		m_assetChangedQueue.Emplace(assetHandle, state);
	}

	void AssetManager_New::IterateAssetRegistryWithFilter(const AssetRegistryIteratorFilter& filter, AssetRegistryIteratorFunc&& func) const
	{
		VT_ENSURE(func != nullptr);

		for (AssetRegistryConstIterator it(m_assetRegistry); it; ++it)
		{
			ReadOnlyAssetMetadata assetMetadata = *it;

			if (!filter.includeMemoryAssets && assetMetadata->isMemoryAsset)
			{
				continue;
			}

			if (!filter.includeWithoutFilepath && !assetMetadata->isMemoryAsset && !assetMetadata->HasFilepath())
			{
				continue;
			}

			if (!filter.filteredAssetTypes.empty() && !filter.filteredAssetTypes.contains(assetMetadata->type))
			{
				continue;
			}

			if (!func(*it))
			{
				break;
			}
		}
	}

	std::filesystem::path AssetManager_New::GetContextPath(const std::filesystem::path& path) const
	{
		std::filesystem::path projDir;

		if (!IsEngineAsset(path))
		{
			projDir = m_root.projectDirectoryPath;
		}

		return projDir;
	}

	std::filesystem::path AssetManager_New::GetFilesystemPath(const std::filesystem::path& path) const
	{
		return GetContextPath(path) / path;
	}

	std::filesystem::path AssetManager_New::GetFilesystemPath(AssetHandle assetHandle) const
	{
		ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(assetHandle);
		return GetFilesystemPath(assetMetadata->filepath);
	}

	std::filesystem::path AssetManager_New::GetRelativeAssetFilepath(const std::filesystem::path& path) const
	{
		return m_assetRegistry.GetRelativeAssetFilepath(path);
	}

	bool AssetManager_New::IsEngineAsset(const std::filesystem::path& path) const
	{
		const auto pathSplit = ::Utility::SplitStringsByCharacter(path.string(), '/');
		if (!pathSplit.empty())
		{
			std::string lowerFirstPart = ::Utility::ToLower(pathSplit.front());
			if (::Utility::StringContains(lowerFirstPart, "engine") || ::Utility::StringContains(lowerFirstPart, "editor"))
			{
				return true;
			}
		}

		return false;
	}

	void AssetManager_New::LoadAsset(AssetHandle assetHandle, RefPtr<Asset_New> asset)
	{
		ScopedTimer timer{};

		m_dependencyGraph->AddAssetToGraph(assetHandle);

		{
			ReadOnlyAssetMetadata readOnlyAssetMetadata = GetReadOnlyAssetMetadata(assetHandle);
			AssetSerializerRegistry::Get().GetSerializer(asset->GetType()).Deserialize(readOnlyAssetMetadata, asset);
		}

		m_assetCache.AddAsset(asset);

		WriteableAssetMetadata assetMetadata = GetWriteableAssetMetadata(assetHandle);
		assetMetadata->isLoaded = true;

		m_dependencyGraph->OnAssetChanged(assetHandle, AssetChangedState::Loaded);

		VT_LOGC(Trace, LogAssetSystem, "Loaded asset {0} with handle {1} in {2} seconds!", assetMetadata->filepath, assetMetadata->handle, timer.GetTime<Time::Seconds>());
	}

	void AssetManager_New::UnloadAndFreeAsset(AssetRefCounter* assetRefCounter)
	{
		// Safe to upcast like this, because AssetRefCounter should only be derived by Asset.
		Asset_New* asset = reinterpret_cast<Asset_New*>(assetRefCounter);
		const AssetHandle assetHandle = asset->GetAssetHandle();
		const std::string nameCopy(asset->GetAssetName());

		const AssetType assetType = asset->GetType();

		// Make sure we lock the metadata
		if (m_assetRegistry.IsValidAssetHandle(assetHandle))
		{
			AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);

			// Lock metadata mutex here.
			assetMetadata->m_assetMetadataMutex.lock();

			m_assetCache.RemoveAsset(assetHandle);

			// At this point there should be zero references left.
			VT_ENSURE(asset->GetRefCount() == 0);

			// Call destructor and free.
			asset->~Asset_New();
			m_assetAllocator.FreeAsset(assetType, asset);

			if (!assetMetadata->isMemoryAsset)
			{
				assetMetadata->isLoaded = false;
				assetMetadata->isQueued = false;

				// Unlock it here as we are finished with it.
				assetMetadata->m_assetMetadataMutex.unlock();
			}
			else
			{
				// If the asset is a memory asset, we will also remove it from the registry.
				// There is no reason to keep it around.
				// The mutex gets unlocked in here.
				m_assetRegistry.RemoveAssetMetadata(assetHandle, true);
			}
		}
		// The asset has been removed from the registry, just destroy it.
		else
		{
			// At this point there should be zero references left.
			VT_ENSURE(asset->GetRefCount() == 0);

			asset->~Asset_New();
			m_assetAllocator.FreeAsset(assetType, asset);
		}
		
		VT_LOGC(Trace, LogAssetSystem, "Asset '{}' (Handle: '{}', Type: '{}') was unloaded!", nameCopy, assetHandle, assetType->GetName());
	}

	void AssetManager_New::CreateDependencyGraphAndAddAssetsFromRegistry()
	{
		m_dependencyGraph = CreateScope<AssetDependencyGraph>();
	
		for (AssetRegistryConstIterator it(m_assetRegistry); it; ++it)
		{
			m_dependencyGraph->AddAssetToGraph((*it)->handle);
		}
	}

	ReadOnlyAssetMetadata AssetManager_New::GetAssetMetadataFromFilepath(const std::filesystem::path& filepath)
	{
		AssetRegistryIteratorFilter filter{};

		ReadOnlyAssetMetadata resultAssetMetadata{ AssetMetadataInit::Null };

		IterateAssetRegistryWithFilter(filter, [filepath, &resultAssetMetadata](ReadOnlyAssetMetadata assetMetadata)
		{
			if (assetMetadata->filepath == filepath)
			{
				resultAssetMetadata = assetMetadata;
				return false;
			}

			return true;
		});

		return { AssetMetadataInit::Null };
	}

	AssetHandle AssetManager_New::GetAssetHandleFromFilepath(const std::filesystem::path& filepath) const
	{
		AssetRegistryIteratorFilter filter{};
		filter.includeMemoryAssets = false;

		AssetHandle resultAssetHandle = Asset_New::Null();

		std::filesystem::path relativeFilepath = GetRelativeAssetFilepath(filepath);

		IterateAssetRegistryWithFilter(filter, [&resultAssetHandle, relativeFilepath](ReadOnlyAssetMetadata assetMetadata)
		{
			if (assetMetadata->filepath == relativeFilepath)
			{
				resultAssetHandle = assetMetadata->handle;
				return false;
			}

			return true;
		});

		return resultAssetHandle;
	}

	AssetManager_New::ScopedAssetLock::ScopedAssetLock(RefPtr<Asset_New> asset)
		: m_asset(asset)
	{
		m_asset->m_assetMutex.lock();
	}

	AssetManager_New::ScopedAssetLock::~ScopedAssetLock()
	{
		m_asset->m_assetMutex.unlock();
	}
}
