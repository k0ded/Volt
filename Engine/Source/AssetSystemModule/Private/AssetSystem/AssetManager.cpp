#include "aspch.h"
#include "AssetManager.h"

#include <CoreUtilities/ConsoleVariableRegistry.h>

#include <FileSystemModule/FileIORequest.h>
#include <FileSystemModule/Filesystem.h>
#include <FileSystemModule/IOThreads/IOThreads.h>

#include <JobSystem/JobSystem.h>

#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Locks/ScopedLock.h>

Unique<Volt::AssetManager> g_assetManager;

namespace Volt
{
	VT_DEFINE_LOG_CATEGORY(LogAssetSystem);

	AssetManager::AssetManager(const Filesystem::Path& engineDirectoryPath, const Filesystem::Path& projectDirectoryPath, StringView assetsDirectoryName)
		: m_assetRegistry(engineDirectoryPath, projectDirectoryPath, assetsDirectoryName)
	{
		RegisterListener<AppTickEvent>(VT_BIND_EVENT_FN(AssetManager::UpdateInternal));

		m_root.engineDirectoryPath = engineDirectoryPath;
		m_root.projectDirectoryPath = projectDirectoryPath;
		m_root.assetsDirectoryName = assetsDirectoryName;

		CreateDependencyGraphAndAddAssetsFromRegistry();
		m_assetChangedQueue.Allocate(4096);
		m_assetDestructionQueue.Allocate(4096);
		m_assetEvictionQueue.Allocate(4096);
	}

	AssetManager::~AssetManager()
	{
		RunGarbageCollection(0, true);
		m_assetCache.Clear();
	}

	VTAS_API int32_t AssetManager::GetNumAssetsInRegistry() const
	{
		return m_assetRegistry.GetNumMetadata();
	}

	WriteableAssetMetadata AssetManager::GetWriteableAssetMetadata(AssetHandle assetHandle) const
	{
		if (assetHandle == Asset::Null())
		{
			return { nullptr };
		}

		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		return { assetMetadata };
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

		AssetMetadata* metadata = m_assetRegistry.GetAssetMetadata(assetHandle);

		AssetLoadState expectedLoadState = AssetLoadState::Loaded;
		if (!metadata->TryTransitionLoadState(expectedLoadState, AssetLoadState::Unloading))
		{
			VT_LOGC(Warning, LogAssetSystem, "Tried to reload asset with handle '{}', but it is not loaded!", assetHandle);
			return;
		}

		// Bump the generation to invalidate old asset.
		metadata->m_generation.fetch_add(1, std::memory_order::acq_rel);

		bool wasCreated = false;
		IntRef<Asset> newAsset = TryCreateAsset(assetHandle, AssetLoadState::Unloading, AssetLoadState::Queued, wasCreated);

		if (wasCreated)
		{
			// The asset was created by this thread, let's queue it for load.
			QueueAssetForLoading(assetHandle, newAsset, AssetLoadState::Queued);
		}

		VT_LOGC(Trace, LogAssetSystem, "Reloaded asset '{}' (Handle: '{}', Type: '{}')!", newAsset->GetAssetName(), newAsset->GetAssetHandle(), newAsset->GetType()->GetName());
	}

	void AssetManager::SaveAsset(AssetHandle assetHandle)
	{
		AssetMetadata* metadata = m_assetRegistry.GetAssetMetadata(assetHandle);

		if (metadata != nullptr)
		{
			IntRef<Asset> asset = TryGetOrTryWaitForPublishedAsset(assetHandle);
			if (asset)
			{
				SaveAsset(AssetReference<Asset>(asset));
			}
			else
			{
				VT_LOGC(Warning, LogAssetSystem, "Tried to save asset with handle '{}', but it is not loaded!", assetHandle);
			}
		}
		else
		{
			VT_LOGC(Warning, LogAssetSystem, "Unable to save asset with handle '{}', is is not valid!", assetHandle);
		}
	}

	void AssetManager::SaveAsset(AssetReference<Asset> asset)
	{
		if (!asset->IsValid())
		{
			VT_LOGC(Error, LogAssetSystem, "Unable to save invalid asset '{0}' (Handle: '{1}')!", asset->GetAssetName(), asset->GetAssetHandle());
			return;
		}

		{
			ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(asset->GetAssetHandle());

			if (assetMetadata->IsMemoryAsset())
			{
				VT_LOGC(Error, LogAssetSystem, "Tried to save an asset '{0}' (Handle: '{1}') that is a memory asset. ", asset->GetAssetName(), asset->GetAssetHandle());
				return;
			}

			if (assetMetadata->filepath.IsEmpty())
			{
				VT_LOGC(Error, LogAssetSystem, "Tried to save an asset '{0}' (Handle: '{1}') that that does not have a path. ", asset->GetAssetName(), asset->GetAssetHandle());
				return;
			}
		}

		{
			ScopedTimer timer{};

			{
				WriteableAssetMetadata assetMetadata = GetWriteableAssetMetadata(asset->GetAssetHandle());
				asset->OnPreSave(assetMetadata->customData);
			}

			{
				AssetMetadata* assetMetadataPtr = m_assetRegistry.GetAssetMetadata(asset->GetAssetHandle());
				AssetDependencyGatherContext gatherContext(assetMetadataPtr->handle, assetMetadataPtr->assetDependencyList);
				asset->GatherAssetDependencies(gatherContext, GetReadOnlyAssetMetadata(asset->GetAssetHandle()));
			
				m_dependencyGraph->ClearAssetDependencies(assetMetadataPtr->handle);
				
				for (const auto& assetDependency : assetMetadataPtr->assetDependencyList.dependencies)
				{
					m_dependencyGraph->AddDependencyToAsset(asset->GetAssetHandle(), assetDependency.assetHandle);
				}
			}

			if (SerializeAsset(asset))
			{
				ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(asset->GetAssetHandle());
				VT_LOGC(Trace, LogAssetSystem, "Saved asset {0} to {1} in {2} seconds!", assetMetadata->handle, assetMetadata->filepath, timer.GetTime<Time::Seconds>());
			}
		}

		m_dependencyGraph->OnAssetChanged(asset->GetAssetHandle(), AssetChangedState::Saved);
		QueueAssetChanged(asset->GetAssetHandle(), AssetChangedState::Saved);
	}

	void AssetManager::RemoveAsset(AssetHandle assetHandle)
	{
		VT_ENSURE(m_assetRegistry.IsValidAssetHandle(assetHandle));

		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		if (assetMetadata)
		{
			m_dependencyGraph->OnAssetChanged(assetHandle, AssetChangedState::Deleted);
			QueueAssetChanged(assetHandle, AssetChangedState::Deleted);

			m_dependencyGraph->RemoveAssetFromGraph(assetHandle);

			AssetMetadata* metadata = m_assetRegistry.RemoveAndGetAssetMetadata(assetHandle);
			{
				ScopedLock lock{ m_assetMetadataReclamationMutex };
				m_assetMetadataReclamationList.emplace_back(metadata, m_frameIndex.load(std::memory_order::relaxed));
			}
		}
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

		IntRef<Asset> tempAsset = TryGetOrTryWaitForPublishedAsset(assetHandle);
		if (!tempAsset)
		{
			return false;
		}

		outAsset = tempAsset;
		return true;
	}

	bool AssetManager::TryGetTypelessAssetImmediately(AssetHandle assetHandle, AssetReference<Asset>& outAsset)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		// Make sure the asset exists.
		if (!m_assetRegistry.IsValidAssetHandle(assetHandle))
		{
			VT_LOGC(Warning, LogAssetSystem, "Asset handle '{}' is not a valid asset handle!", assetHandle);
			return false;
		}

		bool wasCreated = false;
		IntRef<Asset> newAsset = TryCreateAsset(assetHandle, AssetLoadState::Unloaded, AssetLoadState::Loading, wasCreated);

		if (wasCreated)
		{
			// The asset was created by this thread, let's load it.
			LoadAsset(assetHandle, newAsset, AssetLoadState::Loading);
		}

		outAsset = newAsset;
		return newAsset != nullptr;
	}

	bool AssetManager::TryGetTypelessAsset(AssetHandle assetHandle, AssetReference<Asset>& outAsset)
	{
		VT_ENSURE(assetHandle != Asset::Null());

		// Make sure the asset exists.
		if (!m_assetRegistry.IsValidAssetHandle(assetHandle))
		{
			VT_LOGC(Warning, LogAssetSystem, "Asset handle '{}' is not a valid asset handle!", assetHandle);
			return false;
		}

		bool wasCreated = false;
		IntRef<Asset> newAsset = TryCreateAsset(assetHandle, AssetLoadState::Unloaded, AssetLoadState::Queued, wasCreated);

		if (wasCreated)
		{
			// The asset was created by this thread, let's queue it for load.
			QueueAssetForLoading(assetHandle, newAsset, AssetLoadState::Queued);
		}

		AssetMetadata* metadata = m_assetRegistry.GetAssetMetadata(assetHandle);

		outAsset = newAsset;
		return newAsset != nullptr && metadata->IsLoaded();
	}

	AssetReference<Asset> AssetManager::CreateAssetTypeless(StringView assetName, AssetType assetType)
	{
		IntRef<Asset> newAsset = m_assetAllocator.AllocateAssetWithType(assetType);

		AssetMetadata metadata{};
		metadata.filepath = ""; // Assets that are not saved will not have a file path
		metadata.handle = newAsset->GetAssetHandle();
		metadata.type = assetType;
		metadata.m_loadState = AssetLoadState::Loaded;
		metadata.SetFlag(AssetMetadataFlag::MemoryOnly, false);
		metadata.SetFlag(AssetMetadataFlag::Anonymous, false);

		if (CustomAssetMetadataRegistry::Get().AssetTypeHasCustomMetadata(assetType))
		{
			CustomAssetMetadataRegistry::Get().SetupInitalCustomMetadata(assetType, metadata.customData);
		}

		newAsset->SetName(String(assetName));

		// Setup a link back to the asset manager.
		newAsset->m_referencedAssetManager = this;
		newAsset->m_generation = metadata.m_generation;

		m_assetRegistry.InsertAssetMetadata(std::move(metadata));
		AddAssetToCache(newAsset);

		m_dependencyGraph->AddAssetToGraph(newAsset->GetAssetHandle());
		QueueAssetChanged(newAsset->GetAssetHandle(), AssetChangedState::Loaded);

		return newAsset;
	}

	void AssetManager::CreateFileForAsset(AssetHandle assetHandle, const Filesystem::Path& filepath)
	{
		if (Filesystem::FilepathIsOnlyExtension(filepath) || filepath.Stem().IsEmpty())
		{
			VT_LOGC(Error, LogAssetSystem, "No filename was provided while trying to save asset '{0}'. Target file path: '{1}'", assetHandle, filepath);
			return;
		}

		{
			WriteableAssetMetadata assetMetadata = GetWriteableAssetMetadata(assetHandle);
			if (!assetMetadata.IsValid())
			{
				VT_LOGC(Error, LogAssetSystem, "Tried to create a file for an asset '{0}' that is not registered in the asset registry. Target file path: '{1}'", assetHandle, filepath);
				return;
			}

			if (assetMetadata->IsMemoryAsset())
			{
				VT_LOGC(Error, LogAssetSystem, "Tried to create a file for an asset '{0}' that is marked as a memory asset. Target file path: '{1}'", assetHandle, filepath);
				return;
			}

			if (!assetMetadata->filepath.IsEmpty())
			{
				VT_LOGC(Warning, LogAssetSystem, "Tried to create a file for an asset '{0}' that already has an assigned file path, overriding!. Target file path: '{1}'", assetHandle, filepath);
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
		m_frameIndex.store(e.GetFrameIndex(), std::memory_order::relaxed);
		RunGarbageCollection(e.GetFrameIndex(), false);

		{
			std::scoped_lock lock{ m_assetCallbackMutex };

			AssetChangedQueueInfo info;
			while (m_assetChangedQueue.Pop(info))
			{
				OnAssetChanged(info.handle, info.state);
			}
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

	Filesystem::Path AssetManager::GetContextPath(const Filesystem::Path& path) const
	{
		Filesystem::Path projDir;

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

	Filesystem::Path AssetManager::GetAssetFilesystemPath(const Filesystem::Path& path) const
	{
		if (path.IsAbsolute())
		{
			return path;
		}

		return GetContextPath(path) / path;
	}

	Filesystem::Path AssetManager::GetAssetFilesystemPath(AssetHandle assetHandle) const
	{
		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(assetHandle);
		return GetAssetFilesystemPath(assetMetadata->filepath);
	}

	Filesystem::Path AssetManager::GetRelativeAssetFilepath(const Filesystem::Path& path) const
	{
		return m_assetRegistry.GetRelativeAssetFilepath(path);
	}

	bool AssetManager::IsEngineAsset(const Filesystem::Path& path) const
	{
		const auto pathSplit = ::Utility::SplitStringsByCharacter(path.ToString(), '/');
		if (!pathSplit.empty())
		{
			String lowerFirstPart = ::Utility::ToLower(pathSplit.front());
			if (lowerFirstPart.contains("engine") || lowerFirstPart.contains("editor"))
			{
				return true;
			}
		}

		return false;
	}

	JobCounterRef AssetManager::GetMetadataLoadingCounter()
	{
		return m_assetRegistry.GetMetadataLoadingCounter();
	}

	void AssetManager::LoadAsset(AssetHandle assetHandle, IntRef<Asset> asset, AssetLoadState expectedLoadState)
	{
		VT_PROFILE_FUNCTION();

		ScopedTimer timer{};

		DeserializeAsset(asset);

		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		if (!assetMetadata->TryTransitionLoadState(expectedLoadState, AssetLoadState::Loaded))
		{
			// We should never enter this point, since that means that another thread has
			// changed the state while we were loading.
			VT_ENSURE(false);
		}

		QueueAssetChanged(assetHandle, AssetChangedState::Loaded);
		m_dependencyGraph->OnAssetChanged(assetHandle, AssetChangedState::Loaded);

		VT_LOGC(Trace, LogAssetSystem, "Loaded asset '{}' (Handle: '{}') in {} seconds!", assetMetadata->filepath, assetMetadata->handle, timer.GetTime<Time::Seconds>());
	}

	void AssetManager::QueueAssetForLoading(AssetHandle assetHandle, IntRef<Asset> asset, AssetLoadState expectedLoadState)
	{
		JobRef loadJob = JobSystem::CreateJob("Load Asset", ExecutionPriority::Latent, [this, asset, assetHandle, expectedLoadState]()
		{
			ScopedTimer timer{};

			DeserializeAsset(asset);

			AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);

			AssetLoadState tempExpectedLoadState = expectedLoadState;
			if (!assetMetadata->TryTransitionLoadState(tempExpectedLoadState, AssetLoadState::Loaded))
			{
				// We should never enter this point, since that means that another thread has
				// changed the state while we were loading.
				VT_ENSURE(false);
			}

			QueueAssetChanged(assetHandle, AssetChangedState::Loaded);
			m_dependencyGraph->OnAssetChanged(assetHandle, AssetChangedState::Loaded);

			VT_LOGC(Trace, LogAssetSystem, "Loaded asset '{}' (Handle: '{}') in {} seconds!", assetMetadata->filepath, assetMetadata->handle, timer.GetTime<Time::Seconds>());
		}, FiberStackSize::KB128);

		JobSystem::RunJob(loadJob);

		VT_LOGC(Trace, LogAssetSystem, "Queued asset '{}' (Handle: '{}') for loading!", asset->GetAssetName(), asset->GetAssetHandle());
	}

	IntRef<Asset> AssetManager::TryCreateAsset(AssetHandle assetHandle, AssetLoadState expectedLoadState, AssetLoadState dstLoadState, bool& wasCreated)
	{
		AssetMetadata* metadata = m_assetRegistry.GetAssetMetadata(assetHandle);

		uint64_t currentGeneration = metadata->GetGeneration(std::memory_order::acquire);

		if (!metadata->TryTransitionLoadState(expectedLoadState, dstLoadState))
		{
			// The asset was not in the unloaded state.
			wasCreated = false;
			return TryGetOrTryWaitForPublishedAsset(assetHandle);
		}

		// The asset should now be created and loaded.

		IntRef<Asset> newAsset = m_assetAllocator.AllocateAssetWithType(metadata->type);
		// Setup a link back to the asset manager.
		newAsset->m_referencedAssetManager = this;
		newAsset->m_generation = currentGeneration;
		newAsset->AssignAssetHandle(metadata->handle);
		newAsset->SetName(metadata->filepath.Stem().ToString());

		AddAssetToCache(newAsset);
		m_dependencyGraph->AddAssetToGraph(assetHandle);

		wasCreated = true;

		return newAsset;
	}

	void AssetManager::AddAssetToCache(IntRef<Asset> asset)
	{
		if (!m_assetCache.TryPublish(asset->GetAssetHandle(), asset, asset->m_generation))
		{
			// Should always succeed.
			VT_ENSURE(false);
		}

		AssetMetadata* metadata = m_assetRegistry.GetAssetMetadata(asset->GetAssetHandle());
		metadata->m_publishedGeneration.store(asset->m_generation, std::memory_order::release);
		metadata->m_publishedGeneration.notify_all();
	}

	IntRef<Asset> AssetManager::TryGetOrTryWaitForPublishedAsset(AssetHandle assetHandle)
	{
		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		uint64_t currentGeneration = assetMetadata->GetGeneration(std::memory_order::acquire);

		IntRef<Asset> resultAsset;
		if (m_assetCache.TryGet(assetHandle, currentGeneration, resultAsset))
		{
			return resultAsset;
		}

		while (true)
		{
			if (assetMetadata->GetGeneration(std::memory_order::acquire) != currentGeneration)
			{
				return nullptr;
			}

			// The asset was/is unloaded, stop.
			const AssetLoadState currentLoadState = assetMetadata->m_loadState.load(std::memory_order::acquire);

			if (currentLoadState == AssetLoadState::Unloaded)
			{
				return nullptr;
			}

			uint64_t publishedGeneration = assetMetadata->m_publishedGeneration.load(std::memory_order::acquire);

			if (publishedGeneration >= currentGeneration)
			{
				if (m_assetCache.TryGet(assetHandle, currentGeneration, resultAsset))
				{
					return resultAsset;
				}
				else
				{
					return nullptr;
				}
			}

			assetMetadata->m_publishedGeneration.wait(publishedGeneration, std::memory_order::acquire);
		}
	}

	void AssetManager::QueueAssetForEviction(AssetRefCounter* assetRefCounter)
	{
		VT_CHECK(m_assetEvictionQueue.Emplace(assetRefCounter));
	}

	bool AssetManager::DeserializeAsset(AssetReference<Asset> asset)
	{
		VT_PROFILE_FUNCTION();

		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(asset->GetAssetHandle());

		const Filesystem::Path filepath = GetAssetFilesystemPath(assetMetadata->filepath);

		if (!Filesystem::Exists(filepath))
		{
			VT_LOGC(Error, LogAssetSystem,
				"Failed to load asset '{}' (Handle: '{}', Type: '{}')\n"
				"		Error: The filepath does not exist.",
				filepath,
				assetMetadata->handle,
				assetMetadata->type->GetName());
			asset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		IORequestResult<IORequestReadFile_FileReader> result = IOThreads::SubmitRequest<IORequestReadFile_FileReader>("Read Asset File", filepath);
		FileReader& fileReader = result.GetResult();
		
		if (result.GetResultCode() == IORequestResultCode::Failure)
		{
			VT_LOGC(Error, LogAssetSystem,
				"Failed to load asset '{}' (Handle: '{}', Type: '{}')\n"
				"		Error: {}",
				filepath,
				assetMetadata->handle,
				assetMetadata->type->GetName(),
				fileReader.GetError());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		// Load the asset header and verify the asset.
		AssetMetadata storedAssetMetadata;
		AssetRegistry::AssetHeaderDeserializationResult assetHeaderResult = AssetRegistry::DeserializeAssetHeader(fileReader, storedAssetMetadata, asset->GetVersion(), true);

		if (assetHeaderResult == AssetRegistry::AssetHeaderDeserializationResult::InvalidAssetFile)
		{
			VT_LOGC(Error, LogAssetSystem,
				"Failed to load asset '{}' (Handle: '{}', Type: '{}')\n"
				"		Error: Invalid asset file.",
				filepath,
				assetMetadata->handle,
				assetMetadata->type->GetName());
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}
		else if (assetHeaderResult == AssetRegistry::AssetHeaderDeserializationResult::InvalidVersion)
		{
			VT_LOGC(Warning, LogAssetSystem, "Asset '{}' (Handle: '{}', Type: '{}', Current Version: '{}') has a different version in the file, might not load correctly!",
				filepath,
				assetMetadata->handle,
				assetMetadata->type->GetName(),
				asset->GetVersion());
		}

		if (storedAssetMetadata.handle != assetMetadata->handle)
		{
			VT_LOGC(Error, LogAssetSystem,
				"Failed to load asset '{}' (Handle: '{}', Type: '{}')\n"
				"		Error: Asset Handle mismatch! Expected: {}, Actual: {}.",
				filepath,
				assetMetadata->handle,
				assetMetadata->type->GetName(),
				assetMetadata->handle,
				storedAssetMetadata.handle);

			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		if (storedAssetMetadata.type != assetMetadata->type)
		{
			VT_LOGC(Error, LogAssetSystem,
				"Failed to load asset '{}' (Handle: '{}', Type: '{}')\n"
				"		Error: Asset Type mismatch! Expected: {}, Actual: {}.",
				filepath,
				assetMetadata->handle,
				assetMetadata->type->GetName(),
				assetMetadata->type->GetName(),
				storedAssetMetadata.type->GetName());

			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		// Deserialize the asset.
		{
			VT_PROFILE_SCOPE("Asset Serialize");
			asset->Serialize(fileReader, assetMetadata);
		}
		return true;
	}

	void AssetManager::RunGarbageCollection(uint64_t frameIndex, bool forceCleanupAll)
	{
		constexpr uint64_t Quiescence = 3;

		AssetRefCounter* assetRefCounter = nullptr;
		while (m_assetEvictionQueue.Pop(assetRefCounter))
		{
			// Try to evict the asset
			int32_t expected = 1;
			if (assetRefCounter->m_refCount.compare_exchange_strong(expected, 0,
				std::memory_order::acq_rel,
				std::memory_order::acquire))
			{
				Asset* asset = reinterpret_cast<Asset*>(assetRefCounter);
				AssetHandle assetHandle = asset->GetAssetHandle();

				// Eviction successful, at this point this asset is guaranteed to never be reused again.
				AssetCache::Container* cacheContainer = m_assetCache.Evict(assetHandle, asset->m_generation);

				// The asset metadata may have been removed at this point (if RemoveAsset was called)
				// In the case it wasn't, we will transition the asset metadata state to unloaded.
				AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
				if (assetMetadata)
				{
					AssetLoadState expectedLoadState = AssetLoadState::Loaded;
					if (!assetMetadata->TryTransitionLoadState(expectedLoadState, AssetLoadState::Unloading))
					{
						VT_ENSURE(false);
					}
				}

				m_dependencyGraph->OnAssetChanged(assetHandle, AssetChangedState::Unloaded);
				QueueAssetChanged(assetHandle, AssetChangedState::Unloaded);

				// Finally transition to unloaded.
				if (assetMetadata)
				{
					AssetLoadState expectedLoadState = AssetLoadState::Unloading;
					if (!assetMetadata->TryTransitionLoadState(expectedLoadState, AssetLoadState::Unloaded))
					{
						VT_ENSURE(false);
					}
				}

				m_assetReclamationList.emplace_back(asset, cacheContainer, frameIndex);
			}
		}

		for (int32_t i = static_cast<int32_t>(m_assetReclamationList.size()) - 1; i >= 0; --i)
		{
			EvictionEntry& entry = m_assetReclamationList[i];

			if (entry.evictedOnFrameIndex + Quiescence <= frameIndex || forceCleanupAll)
			{
				AssetType assetType = entry.asset->GetType();
				entry.asset->~Asset();

				m_assetAllocator.FreeAsset(assetType, entry.asset);
				m_assetCache.FreeContainer(entry.cacheContainer);

				m_assetReclamationList.erase_unsorted(m_assetReclamationList.begin() + i);
			}
		}

		{
			ScopedLock lock{ m_assetMetadataReclamationMutex };
			for (int32_t i = static_cast<int32_t>(m_assetMetadataReclamationList.size()) - 1; i >= 0; --i)
			{
				AssetMetadataReclamationEntry& entry = m_assetMetadataReclamationList[i];
				if (entry.evictedOnFrameIndex + Quiescence <= frameIndex || forceCleanupAll)
				{
					m_assetRegistry.FreeAssetMetadata(entry.assetMetadata);
					m_assetMetadataReclamationList.erase_unsorted(m_assetMetadataReclamationList.begin() + i);
				}
			}
		}
	}

	bool AssetManager::SerializeAsset(AssetReference<Asset> asset)
	{
		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(asset->GetAssetHandle());

		if (!assetMetadata->HasFilepath())
		{
			VT_LOGC(Error, LogAssetSystem,
				"Unable to save asset '{}' (Handle: '{}', Type: '{}')\n"
				"		Error: It does not have a filepath!",
				asset->GetAssetName(),
				assetMetadata->handle,
				assetMetadata->type->GetName());
			return false;
		}

		const Filesystem::Path destinationFilepath = GetAssetFilesystemPath(assetMetadata->filepath);

		FileWriter fileWriter;
		if (!fileWriter.Open(destinationFilepath))
		{
			VT_LOGC(Error, LogAssetSystem,
				"Unable to save asset '{}' (Handle: '{}', Type: '{}')\n"
				"		Error: {}",
				asset->GetAssetName(),
				assetMetadata->handle,
				assetMetadata->type->GetName(),
				fileWriter.GetError());

			return false;
		}

		SerializeAssetHeader(fileWriter, *assetMetadata, asset->GetVersion());
		asset->Serialize(fileWriter, assetMetadata);

		IOThreads::SubmitRequest<IORequestWriteFile_FileWriter>("Write Asset File", std::move(fileWriter));
		return true;
	}

	void AssetManager::SerializeAssetHeader(Archive& archive, AssetMetadata assetMetadata, uint32_t assetVersion)
	{
		uint32_t assetMagic = AssetFileMagic;

		archive << assetMagic;
		archive << assetVersion;
		archive << assetMetadata;
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
			if (m_assetChangedCallbacks.contains(AssetTypes::None))
			{
				broadcast(AssetTypes::None);
			}
		}
	}

	void AssetManager::CreateDependencyGraphAndAddAssetsFromRegistry()
	{
		m_dependencyGraph = CreateUnique<AssetDependencyGraph>(*this);

		// Add all engine assets to the graph.
		{
			// Add all assets to the graph
			for (AssetRegistryConstIterator it(m_assetRegistry); it; ++it)
			{
				ReadOnlyAssetMetadata assetMetadata = *it;
				if (assetMetadata->isEngineAsset)
				{
					m_dependencyGraph->AddAssetToGraph((*it)->handle);
				}
			}

			// Link all dependencies
			for (AssetRegistryConstIterator it(m_assetRegistry); it; ++it)
			{
				ReadOnlyAssetMetadata assetMetadata = *it;
				if (assetMetadata->isEngineAsset)
				{
					for (const AssetDependency& dependency : assetMetadata->assetDependencyList.dependencies)
					{
						m_dependencyGraph->AddDependencyToAsset(assetMetadata->handle, dependency.assetHandle);
					}
				}
			}
		};

		// Add all non-engine assets to the graph.
		JobRef insertIntoDependencyGraphJob = JobSystem::CreateJob("Insert Assets Into Dependency Graph", ExecutionPriority::Latent, ExecutionPolicy::MainThread, nullptr, m_assetRegistry.GetMetadataLoadingCounter(), [&]() 
		{
			// Add all assets to the graph
			for (AssetRegistryConstIterator it(m_assetRegistry); it; ++it)
			{
				ReadOnlyAssetMetadata assetMetadata = *it;
				if (!assetMetadata->isEngineAsset)
				{
					m_dependencyGraph->AddAssetToGraph((*it)->handle);
				}
			}

			// Link all dependencies
			for (AssetRegistryConstIterator it(m_assetRegistry); it; ++it)
			{
				ReadOnlyAssetMetadata assetMetadata = *it;
				if (!assetMetadata->isEngineAsset)
				{
					for (const AssetDependency& dependency : assetMetadata->assetDependencyList.dependencies)
					{
						m_dependencyGraph->AddDependencyToAsset(assetMetadata->handle, dependency.assetHandle);
					}
				}
			}
		});

		JobSystem::RunJob(insertIntoDependencyGraphJob);
	}

	ReadOnlyAssetMetadata AssetManager::GetAssetMetadataFromFilepath(const Filesystem::Path& filepath)
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

		return resultAssetMetadata;
	}

	AssetHandle AssetManager::GetAssetHandleFromFilepath(const Filesystem::Path& filepath) const
	{
		AssetRegistryIteratorFilter filter{};
		filter.includeMemoryAssets = false;

		AssetHandle resultAssetHandle = Asset::Null();

		Filesystem::Path relativeFilepath = GetRelativeAssetFilepath(filepath);

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
}
