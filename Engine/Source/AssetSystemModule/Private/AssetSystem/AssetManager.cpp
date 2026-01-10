#include "aspch.h"
#include "AssetManager.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <JobSystem/JobSystem.h>

#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/Archive/FileArchive.h>
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
		m_assetChangedQueue.Allocate(4096);
		m_assetDestructionQueue.Allocate(4096);
	}

	AssetManager::~AssetManager()
	{
		FlushDestructionQueue();
		m_assetCache.Clear();
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
		RefPtr<Asset> newAsset = TryCreateAsset(assetHandle, AssetLoadState::Unloading, AssetLoadState::Queued, wasCreated);

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
			RefPtr<Asset> asset = TryGetOrTryWaitForPublishedAsset(assetHandle);
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

			if (assetMetadata->filepath.empty())
			{
				VT_LOGC(Error, LogAssetSystem, "Tried to save an asset '{0}' (Handle: '{1}') that that does not have a path. ", asset->GetAssetHandle(), asset->GetAssetHandle());
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
			m_assetRegistry.RemoveAssetMetadata(assetHandle);
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

		RefPtr<Asset> tempAsset = TryGetOrTryWaitForPublishedAsset(assetHandle);
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
		RefPtr<Asset> newAsset = TryCreateAsset(assetHandle, AssetLoadState::Unloaded, AssetLoadState::Loading, wasCreated);

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
		RefPtr<Asset> newAsset = TryCreateAsset(assetHandle, AssetLoadState::Unloaded, AssetLoadState::Queued, wasCreated);

		if (wasCreated)
		{
			// The asset was created by this thread, let's queue it for load.
			QueueAssetForLoading(assetHandle, newAsset, AssetLoadState::Queued);
		}

		AssetMetadata* metadata = m_assetRegistry.GetAssetMetadata(assetHandle);

		outAsset = newAsset;
		return newAsset != nullptr && metadata->IsLoaded();
	}

	AssetReference<Asset> AssetManager::CreateAssetTypeless(std::string_view assetName, AssetType assetType)
	{
		RefPtr<Asset> newAsset = m_assetAllocator.AllocateAssetWithType(assetType);

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

		newAsset->SetName(std::string(assetName));

		// Setup a link back to the asset manager.
		newAsset->m_referencedAssetManager = this;
		newAsset->m_generation = metadata.m_generation;

		m_assetRegistry.InsertAssetMetadata(std::move(metadata));
		AddAssetToCache(newAsset);

		m_dependencyGraph->AddAssetToGraph(newAsset->GetAssetHandle());
		QueueAssetChanged(newAsset->GetAssetHandle(), AssetChangedState::Loaded);

		return newAsset;
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
		m_frameIndex = e.GetFrameIndex();

		{
			std::scoped_lock lock{ m_assetCallbackMutex };

			AssetChangedQueueInfo info;
			while (m_assetChangedQueue.Pop(info))
			{
				OnAssetChanged(info.handle, info.state);
			}
		}

		FlushDestructionQueue();

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
		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(assetHandle);
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

	JobCounterRef AssetManager::GetMetadataLoadingCounter()
	{
		return m_assetRegistry.GetMetadataLoadingCounter();
	}

	void AssetManager::LoadAsset(AssetHandle assetHandle, RefPtr<Asset> asset, AssetLoadState expectedLoadState)
	{
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

	void AssetManager::QueueAssetForLoading(AssetHandle assetHandle, RefPtr<Asset> asset, AssetLoadState expectedLoadState)
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
		});

		JobSystem::RunJob(loadJob);

		VT_LOGC(Trace, LogAssetSystem, "Queued asset '{}' (Handle: '{}') for loading!", asset->GetAssetName(), asset->GetAssetHandle());
	}

	RefPtr<Asset> AssetManager::TryCreateAsset(AssetHandle assetHandle, AssetLoadState expectedLoadState, AssetLoadState dstLoadState, bool& wasCreated)
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

		RefPtr<Asset> newAsset = m_assetAllocator.AllocateAssetWithType(metadata->type);
		// Setup a link back to the asset manager.
		newAsset->m_referencedAssetManager = this;
		newAsset->m_generation = currentGeneration;
		newAsset->AssignAssetHandle(metadata->handle);
		newAsset->SetName(metadata->filepath.stem().string());

		AddAssetToCache(newAsset);
		m_dependencyGraph->AddAssetToGraph(assetHandle);

		wasCreated = true;

		return newAsset;
	}

	void AssetManager::AddAssetToCache(RefPtr<Asset> asset)
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

	RefPtr<Asset> AssetManager::TryGetOrTryWaitForPublishedAsset(AssetHandle assetHandle)
	{
		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);
		uint64_t currentGeneration = assetMetadata->GetGeneration(std::memory_order::acquire);

		RefPtr<Asset> resultAsset;
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

	void AssetManager::QueueAssetForDestruction(AssetRefCounter* assetRefCounter)
	{
		Asset* asset = reinterpret_cast<Asset*>(assetRefCounter);

		AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(asset->GetAssetHandle());

		// This is an old instance, it shouldn't change any state on the metadata.
		if (asset->m_generation < assetMetadata->GetGeneration())
		{
			// We'll just queue it for destruction.
			m_assetDestructionQueue.Emplace(assetRefCounter);
		}
		else
		{
			AssetLoadState expectedLoadState = AssetLoadState::Loaded;
			if (!assetMetadata->TryTransitionLoadState(expectedLoadState, AssetLoadState::Unloading))
			{
				VT_ENSURE(false);
			}

			uint64_t oldGeneration = assetMetadata->m_generation.fetch_add(1, std::memory_order::acq_rel);

			VT_CHECK(m_assetCache.TryRemove(assetMetadata->handle, oldGeneration));

			m_dependencyGraph->OnAssetChanged(assetMetadata->handle, AssetChangedState::Unloaded);
			QueueAssetChanged(assetMetadata->handle, AssetChangedState::Unloaded);

			expectedLoadState = AssetLoadState::Unloading;
			if (!assetMetadata->TryTransitionLoadState(expectedLoadState, AssetLoadState::Unloaded))
			{
				VT_ENSURE(false);
			}

			m_assetDestructionQueue.Emplace(assetRefCounter);
		}
	}

	void AssetManager::UnloadAndFreeAsset(AssetUnloadData& assetUnloadData)
	{
		// Safe to upcast like this, because AssetRefCounter should only be derived by Asset.
		Asset* asset = reinterpret_cast<Asset*>(assetUnloadData.asset);

		const AssetHandle assetHandle = asset->GetAssetHandle();
		const std::string nameCopy(asset->GetAssetName());

		const AssetType assetType = asset->GetType();

		// Make sure we lock the metadata
		if (m_assetRegistry.IsValidAssetHandle(assetHandle))
		{
			AssetMetadata* assetMetadata = m_assetRegistry.GetAssetMetadata(assetHandle);

			// At this point there should be zero references left.
			VT_ENSURE(asset->GetRefCount() == 0);

			// Call destructor and free.
			asset->~Asset();
			m_assetAllocator.FreeAsset(assetType, asset);

			// Lock metadata mutex here.
			assetMetadata->m_assetMetadataMutex.lock();

			if (!assetMetadata->IsMemoryAsset())
			{
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

			asset->~Asset();
			m_assetAllocator.FreeAsset(assetType, asset);
		}

		VT_LOGC(Trace, LogAssetSystem, "Asset '{}' (Handle: '{}', Type: '{}') was unloaded!", nameCopy, assetHandle, assetType->GetName());
	}

	bool AssetManager::DeserializeAsset(AssetReference<Asset> asset)
	{
		ReadOnlyAssetMetadata assetMetadata = GetReadOnlyAssetMetadata(asset->GetAssetHandle());

		const std::filesystem::path filepath = GetAssetFilesystemPath(assetMetadata->filepath);

		if (!FileSystem::Exists(filepath))
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

		FileReader fileReader;
		if (!fileReader.Open(filepath))
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
		asset->Serialize(fileReader, assetMetadata);
		return true;
	}

	void AssetManager::FlushDestructionQueue()
	{
		VT_PROFILE_FUNCTION();

		AssetUnloadData unloadData;
		while (m_assetDestructionQueue.Pop(unloadData))
		{
			UnloadAndFreeAsset(unloadData);
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

		const std::filesystem::path destinationFilepath = GetAssetFilesystemPath(assetMetadata->filepath);

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

		fileWriter.Close();
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
		m_dependencyGraph = CreateScope<AssetDependencyGraph>(*this);

		// Add all assets to the graph
		for (AssetRegistryConstIterator it(m_assetRegistry); it; ++it)
		{
			m_dependencyGraph->AddAssetToGraph((*it)->handle);
		}

		// Link all dependencies
		for (AssetRegistryConstIterator it(m_assetRegistry); it; ++it)
		{
			ReadOnlyAssetMetadata assetMetadata = *it;

			for (const AssetDependency& dependency : assetMetadata->assetDependencyList.dependencies)
			{
				m_dependencyGraph->AddDependencyToAsset(assetMetadata->handle, dependency.assetHandle);
			}
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
}
