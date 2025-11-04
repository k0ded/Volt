#include "aspch.h"
#include "AssetManager.h"

#include "AssetSystem/AssetLocks.h"

#include <JobSystem/JobSystem.h>

#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/StringUtility.h>

Scope<Volt::AssetManager> g_assetManager;

namespace Volt
{
	VT_DEFINE_LOG_CATEGORY(LogAssetSystem);

	AssetManager::AssetManager(const std::filesystem::path& engineDirectoryPath, const std::filesystem::path& projectDirectoryPath, std::string_view assetsDirectoryName)
		: m_assetRegistry(engineDirectoryPath, projectDirectoryPath, assetsDirectoryName)
	{
		RegisterListener<AppTickEvent>(VT_BIND_EVENT_FN(AssetManager::UpdateInternal));

		m_root.engineDirectoryPath = engineDirectoryPath;
		m_root.projectDirectoryPath = projectDirectoryPath;
		m_root.assetsDirectoryName = assetsDirectoryName;

		CreateDependencyGraphAndAddAssetsFromRegistry();
		m_assetChangedQueue.Allocate(1024);
	}

	AssetManager::~AssetManager()
	{
		m_assetCache.Clear();
	}

	WriteableAssetMetadata AssetManager::GetWriteableAssetMetadata(AssetHandle assetHandle) const
	{
		if (assetHandle == Asset::Null())
		{
			return { nullptr };
		}

		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		return { assetMetadata};
	}

	ReadOnlyAssetMetadata AssetManager::GetReadOnlyAssetMetadata(AssetHandle assetHandle) const
	{
		if (assetHandle == Asset::Null())
		{
			return { nullptr };
		}

 		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		return { assetMetadata };
	}

	AssetMetadata AssetManager::GetAssetMetadataCopy(AssetHandle assetHandle) const
	{
		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		if (assetMetadata)
		{
			return *assetMetadata;
		}

		return {};
	}
	
	void AssetManager::ReloadAsset(AssetHandle assetHandle)
	{
		if (!IsValidAssetHandle(assetHandle))
		{
			VT_LOGC(Warning, LogAssetSystem, "Tried to reload asset with handle '{}', but that is not a valid asset handle!", assetHandle);
			return;
		}

		RefPtr<Asset> asset;
		if (m_assetCache.TryGetAsset(assetHandle, asset))
		{
			ScopedAssetLock assetLock(asset);

			// Get the previous state
			const int32_t assetRefCount = asset->GetRefCount();
			const std::string assetName = asset->m_name;
			const uint8_t assetFlags = asset->m_assetFlags.load(std::memory_order::relaxed);
			std::shared_mutex* assetMutex = asset->m_assetMutex;

			asset->m_refCount = 0;
			m_assetAllocator.ReallocateAsset(asset->GetType(), asset.GetRaw());

			// Restore the state
			asset->m_refCount = assetRefCount;
			asset->m_name = assetName;
			asset->m_assetFlags = assetFlags;
			asset->m_handle = assetHandle;
			asset->m_referencedAssetManager = this;

			// Assign a temp mutex to make sure deserialization works
			asset->m_assetMutex = new std::shared_mutex();

			// Deserialize the asset again.
			{
				ReadOnlyAssetMetadata readOnlyAssetMetadata = GetReadOnlyAssetMetadata(assetHandle);
				AssetSerializerRegistry::Get().GetSerializer(asset->GetType()).Deserialize(readOnlyAssetMetadata, asset);
			}

			// Delete the temp mutex again.
			delete asset->m_assetMutex;
			asset->m_assetMutex = assetMutex;

			m_dependencyGraph->OnAssetChanged(assetHandle, AssetChangedState::Loaded);
			QueueAssetChanged(assetHandle, AssetChangedState::Loaded);

			VT_LOGC(Trace, LogAssetSystem, "Reloaded asset '{}' (Handle: '{}', Type: '{}')!", asset->GetAssetName(), asset->GetAssetHandle(), asset->GetType()->GetName());
		}
		else
		{
			VT_LOGC(Warning, LogAssetSystem, "Tried to reload asset with handle '{}', but it is not loaded!", assetHandle);
		}
	}

	void AssetManager::SaveAsset(AssetHandle assetHandle)
	{
		RefPtr<Asset> asset;
		if (m_assetCache.TryGetAsset(assetHandle, asset))
		{
			SaveAsset(AssetReference<Asset>(asset));
		}
		else
		{
			VT_LOGC(Warning, LogAssetSystem, "Tried to save asset with handle '{}', but it is not loaded!", assetHandle);
		}
	}

	void AssetManager::SaveAsset(AssetReference<Asset> asset)
	{
		ScopedAssetReferenceLock lock{ asset };

		if (!AssetSerializerRegistry::Get().HasSerializer(asset->GetType()))
		{
			VT_LOGC(Warning, LogAssetSystem, "No serializer for asset '{}' (Handle: '{}') with type '{}' was found!", asset->GetAssetName(), asset->GetAssetHandle(), asset->GetType()->GetName());
			return;
		}

		if (!asset->IsValid())
		{
			VT_LOGC(Error, LogAssetSystem, "Unable to save invalid asset '{0}' (Handle: '{1}')!", asset->GetAssetName(), asset->GetAssetHandle());
			return;
		}

		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(asset->GetAssetHandle());

		if (assetMetadata->IsMemoryAsset())
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

		m_dependencyGraph->OnAssetChanged(asset->GetAssetHandle(), AssetChangedState::Saved);
		QueueAssetChanged(asset->GetAssetHandle(), AssetChangedState::Saved);
	}

	void AssetManager::RemoveAsset(AssetHandle assetHandle)
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

	bool AssetManager::IsValidAssetHandle(AssetHandle assetHandle) const
	{
		return m_assetRegistry.IsValidAssetHandle(assetHandle);
	}

	bool AssetManager::IsAssetLoaded(AssetHandle assetHandle) const
	{
		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(assetHandle);
		return assetMetadata->IsLoaded();
	}

	bool AssetManager::TryGetTypelessAssetIfLoaded(AssetHandle assetHandle, AssetReference<Asset>& outAsset)
	{
		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(assetHandle);
		if (!assetMetadata.IsValid())
		{
			return false;
		}
		
		if (assetMetadata->IsLoaded())
		{
			// Try to get the asset from the asset cache.
			RefPtr<Asset> tempAsset;
			if (m_assetCache.TryGetAsset(assetHandle, tempAsset))
			{
				outAsset = { tempAsset };
				return true;
			}
		}

		return false;
	}

	void AssetManager::CreateFileForAsset(AssetHandle assetHandle, const std::filesystem::path& filepath)
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

			if (assetMetadata->IsMemoryAsset())
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

	AssetManager::AssetUpdatedCallbackID AssetManager::RegisterAssetUpdatedCallback(AssetType assetType, AssetChangedCallback&& callback)
	{
		std::scoped_lock lock{ m_assetCallbackMutex };

		UUID64 id = UUID64{};
		m_assetChangedCallbacks[assetType].push_back({ id, std::move(callback) });
		return id;
	}

	void AssetManager::UnregisterAssetUpdatedCallback(AssetType assetType, UUID64 callbackId)
	{
		std::scoped_lock lock{ m_assetCallbackMutex };

		auto& callbacks = m_assetChangedCallbacks[assetType];

		auto it = std::find_if(callbacks.begin(), callbacks.end(), [&](const AssetChangedCallbackInfo& callbackInfo)
		{
			return callbackInfo.id == callbackId;
		});

		if (it != callbacks.end())
		{
			callbacks.erase(it);
		}
	}

	void AssetManager::AddDependencyToAsset(AssetHandle dependant, AssetHandle dependency)
	{
		m_dependencyGraph->AddDependencyToAsset(dependant, dependency);
	}

	Vector<AssetHandle> AssetManager::GetAssetsDependentOn(AssetHandle assetHandle) const
	{
		return {};
	}

	void AssetManager::QueueAssetChanged(AssetHandle assetHandle, AssetChangedState state)
	{
		m_assetChangedQueue.Emplace(assetHandle, state);
	}

	bool AssetManager::UpdateInternal(class AppTickEvent& e)
	{
		std::scoped_lock lock{ m_assetCallbackMutex };

		AssetChangedQueueInfo info;
		while (m_assetChangedQueue.Pop(info))
		{
			OnAssetChanged(info.handle, info.state);
		}

		return false;
	}

	void AssetManager::IterateAssetRegistryWithFilter(const AssetRegistryIteratorFilter& filter, AssetRegistryIteratorFunc&& func) const
	{
		VT_ENSURE(func != nullptr);

		for (AssetRegistryConstIterator it(m_assetRegistry); it; ++it)
		{
			ReadOnlyAssetMetadata assetMetadata = *it;

			// Skip all anonymous assets.
			if (assetMetadata->IsFlagSet(AssetMetadataFlag::Anonymous))
			{
				continue;
			}

			if (!filter.includeMemoryAssets && assetMetadata->IsMemoryAsset())
			{
				continue;
			}

			if (!filter.includeWithoutFilepath && !assetMetadata->IsMemoryAsset() && !assetMetadata->HasFilepath())
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

	std::filesystem::path AssetManager::GetContextPath(const std::filesystem::path& path) const
	{
		std::filesystem::path projDir;

		if (!IsEngineAsset(path))
		{
			projDir = m_root.projectDirectoryPath;
		}
		else
		{
			projDir = m_root.engineDirectoryPath;
		}

		return projDir;
	}

	std::filesystem::path AssetManager::GetAssetFilesystemPath(const std::filesystem::path& path) const
	{
		if (path.is_absolute())
		{
			return path;
		}

		return GetContextPath(path) / path;
	}

	std::filesystem::path AssetManager::GetAssetFilesystemPath(AssetHandle assetHandle) const
	{
		ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(assetHandle);
		return GetAssetFilesystemPath(assetMetadata->filepath);
	}

	std::filesystem::path AssetManager::GetRelativeAssetFilepath(const std::filesystem::path& path) const
	{
		return m_assetRegistry.GetRelativeAssetFilepath(path);
	}

	bool AssetManager::IsEngineAsset(const std::filesystem::path& path) const
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

	void AssetManager::LoadAsset(AssetHandle assetHandle, RefPtr<Asset> asset)
	{
		ScopedTimer timer{};

		m_dependencyGraph->AddAssetToGraph(assetHandle);

		{
			ReadOnlyAssetMetadata readOnlyAssetMetadata = GetReadOnlyAssetMetadata(assetHandle);
			AssetSerializerRegistry::Get().GetSerializer(asset->GetType()).Deserialize(readOnlyAssetMetadata, asset);
		}

		m_assetCache.AddAsset(asset);

		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		assetMetadata->SetFlag(AssetMetadataFlag::Loaded, true);

		QueueAssetChanged(assetHandle, AssetChangedState::Loaded);
		m_dependencyGraph->OnAssetChanged(assetHandle, AssetChangedState::Loaded);

		VT_LOGC(Trace, LogAssetSystem, "Loaded asset '{}' (Handle: '{}') in {} seconds!", assetMetadata->filepath, assetMetadata->handle, timer.GetTime<Time::Seconds>());
	}

	void AssetManager::QueueAssetForLoading(AssetHandle assetHandle, RefPtr<Asset> asset)
	{
		asset->SetFlag(AssetFlag::Queued, true);

		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		assetMetadata->SetFlag(AssetMetadataFlag::Queued, true);

		m_dependencyGraph->AddAssetToGraph(assetHandle);
		m_assetCache.AddAsset(asset);

		JobRef loadJob = JobSystem::CreateJob("Load Asset", ExecutionPriority::Latent, [this, asset, assetHandle]() 
		{
			ScopedTimer timer{};

			{
				ReadOnlyAssetMetadata readOnlyAssetMetadata = GetReadOnlyAssetMetadata(assetHandle);
				AssetSerializerRegistry::Get().GetSerializer(asset->GetType()).Deserialize(readOnlyAssetMetadata, asset);
			}

			asset->SetFlag(AssetFlag::Queued, false);

			AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
			assetMetadata->SetFlag(AssetMetadataFlag::Loaded, true);
			assetMetadata->SetFlag(AssetMetadataFlag::Queued, false);

			QueueAssetChanged(assetHandle, AssetChangedState::Loaded);
			m_dependencyGraph->OnAssetChanged(assetHandle, AssetChangedState::Loaded);

			VT_LOGC(Trace, LogAssetSystem, "Loaded asset '{}' (Handle: '{}') in {} seconds!", assetMetadata->filepath, assetMetadata->handle, timer.GetTime<Time::Seconds>());
		});

		JobSystem::RunJob(loadJob);

		VT_LOGC(Trace, LogAssetSystem, "Queued asset '{}' (Handle: '{}') for loading!", asset->GetAssetName(), asset->GetAssetHandle());
	}

	void AssetManager::UnloadAndFreeAsset(AssetRefCounter* assetRefCounter)
	{
		// Safe to upcast like this, because AssetRefCounter should only be derived by Asset.
		Asset* asset = reinterpret_cast<Asset*>(assetRefCounter);
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
			delete asset->m_assetMutex;
			asset->~Asset();
			m_assetAllocator.FreeAsset(assetType, asset);

			if (!assetMetadata->IsMemoryAsset())
			{
				assetMetadata->SetFlag(AssetMetadataFlag::Loaded, false);
				assetMetadata->SetFlag(AssetMetadataFlag::Queued, false);

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

			delete asset->m_assetMutex;
			asset->~Asset();
			m_assetAllocator.FreeAsset(assetType, asset);
		}
		
		m_dependencyGraph->OnAssetChanged(assetHandle, AssetChangedState::Unloaded);
		QueueAssetChanged(assetHandle, AssetChangedState::Unloaded);
		VT_LOGC(Trace, LogAssetSystem, "Asset '{}' (Handle: '{}', Type: '{}') was unloaded!", nameCopy, assetHandle, assetType->GetName());
	}

	void AssetManager::OnAssetChanged(AssetHandle assetHandle, AssetChangedState state)
	{
		auto broadcast = [&](const AssetType type)
		{
			const auto& callbacks = m_assetChangedCallbacks.at(type);
			for (const auto& callback : callbacks)
			{
				if (!callback.callback)
				{
					continue;
				}

				callback.callback(assetHandle, state);
			}
		};

		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(assetHandle);
		if (assetMetadata.IsValid())
		{
			if (m_assetChangedCallbacks.contains(assetMetadata->type))
			{
				broadcast(assetMetadata->type);
			}

			//also call all the ones registered to AssetTypes::None
			broadcast(AssetTypes::None);
		}
	}

	void AssetManager::CreateDependencyGraphAndAddAssetsFromRegistry()
	{
		m_dependencyGraph = CreateScope<AssetDependencyGraph>(*this);
	
		for (AssetRegistryConstIterator it(m_assetRegistry); it; ++it)
		{
			m_dependencyGraph->AddAssetToGraph((*it)->handle);
		}
	}

	ReadOnlyAssetMetadata AssetManager::GetAssetMetadataFromFilepath(const std::filesystem::path& filepath)
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

	AssetHandle AssetManager::GetAssetHandleFromFilepath(const std::filesystem::path& filepath) const
	{
		AssetRegistryIteratorFilter filter{};
		filter.includeMemoryAssets = false;

		AssetHandle resultAssetHandle = Asset::Null();

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

	AssetManager::ScopedAssetLock::ScopedAssetLock(RefPtr<Asset> asset)
		: m_asset(asset)
	{
		m_asset->m_assetMutex->lock();
	}

	AssetManager::ScopedAssetLock::~ScopedAssetLock()
	{
		m_asset->m_assetMutex->unlock();
	}
}
