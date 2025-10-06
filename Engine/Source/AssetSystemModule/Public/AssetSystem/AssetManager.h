#pragma once

#include "AssetSystem/Asset.h"
#include "AssetSystem/Events/AssetEvents.h"

#include <LogModule/Log.h>

#include <SubSystem/SubSystem.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/StringUtility.h>
#include <CoreUtilities/Profiling/Profiling.h>

#include <EventSystem/EventSystem.h>
#include <EventSystem/EventListener.h>

#include <filesystem>
#include <unordered_map>
#include <shared_mutex>
#include <functional>
#include <concepts>
#include <type_traits>

namespace Volt
{
	VT_DECLARE_LOG_CATEGORY_EXPORT(VTAS_API, LogAssetSystem, LogVerbosity::Trace);

	class AssetFactory;
	class AssetSerializer;
	class AssetDependencyGraph;
	class AppUpdateEvent;

	template<typename T>
	concept IsVoltAsset = std::is_base_of_v<Volt::Asset, T>;

	class VTAS_API AssetManager : public EventListener
	{
	public:
		using WriteLock = std::unique_lock<std::shared_mutex>;
		using ReadLock = std::shared_lock<std::shared_mutex>;
		using AssetCreateFunction = std::function<Ref<Asset>()>;
		using AssetChangedCallback = std::function<void(AssetHandle assetHandle, AssetChangedState state)>;

		using AssetRegistry = Map<AssetHandle, AssetMetadata>;
		using AssetCache = Map<AssetHandle, Ref<Asset>>;

		AssetManager(const std::filesystem::path& projectDirectory, const std::filesystem::path& assetsDirectory, const std::filesystem::path& engineDirectory);
		~AssetManager() override;

		void Initialize();
		void Shutdown();
		void Clear();

		void UnloadAsset(AssetHandle assetHandle);
		void UnloadMemoryAsset(AssetHandle assetHandle);

		void MoveAsset(Ref<Asset> asset, const std::filesystem::path& targetDir);
		void MoveAsset(AssetHandle asset, const std::filesystem::path& targetDir);
		void MoveAssetInRegistry(const std::filesystem::path& sourcePath, const std::filesystem::path& targetPath);
		void MoveFullFolder(const std::filesystem::path& sourceDir, const std::filesystem::path& targetDir);

		void RenameAsset(AssetHandle asset, const std::string& newName);
		void RenameAssetFolder(AssetHandle asset, const std::filesystem::path& targetFilePath);

		void RemoveAsset(AssetHandle asset);
		void RemoveAsset(const std::filesystem::path& path);

		void RemoveAssetFromRegistry(AssetHandle asset);
		void RemoveAssetFromRegistry(const std::filesystem::path& path);
		void RemoveFullFolderFromRegistry(const std::filesystem::path& path);

		void AddAssetToRegistry(const std::filesystem::path& path, AssetHandle handle, AssetType type);
		AssetHandle GetOrAddAssetToRegistry(const std::filesystem::path& path, AssetType type);

		void ReloadAsset(AssetHandle handle);
		void ReloadAsset(const std::filesystem::path& path);

		Ref<Asset> GetAssetRaw(AssetHandle assetHandle);
		Ref<Asset> QueueAssetRaw(AssetHandle assetHandle);

		static UUID64 RegisterAssetUpdatedCallback(AssetType assetType, AssetChangedCallback&& callbackFunction);
		static void UnregisterAssetUpdatedCallback(AssetType assetType, UUID64 id);

		static void AddDependencyToAsset(AssetHandle handle, AssetHandle dependency);
		static Vector<AssetHandle> GetAssetsDependentOn(AssetHandle handle);

		static bool IsLoaded(AssetHandle handle);

		static bool IsEngineAsset(const std::filesystem::path& path);
		static bool IsMemoryAsset(AssetHandle handle);
		static bool ExistsInRegistry(AssetHandle handle);
		static bool ExistsInRegistry(const std::filesystem::path& path);

		static void SaveAsset(AssetHandle handle);

		static const std::filesystem::path GetFilesystemPath(AssetHandle handle);
		static const std::filesystem::path GetFilesystemPath(const std::filesystem::path& path);
		static const std::filesystem::path GetRelativePath(const std::filesystem::path& path);
		static const std::filesystem::path GetFilePathFromAssetHandle(AssetHandle handle);
		static const std::filesystem::path GetContextPath(const std::filesystem::path& path);
		static const bool HasFilePath(AssetHandle handle);

		static AssetType GetAssetTypeFromHandle(const AssetHandle& handle);
		static AssetType GetAssetTypeFromPath(const std::filesystem::path& path);
		static AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& path);

		static AssetMetadata GetMetadataFromHandle(AssetHandle handle);
		static AssetMetadata GetMetadataFromFilePath(const std::filesystem::path filePath);

		static const AssetRegistry& GetAssetRegistry();
		static AssetRegistry& GetAssetRegistryMutable();

		// Is not guaranteed to return the "correct" asset if there are multiple assets with the same name and type
		static const std::filesystem::path GetFilePathFromFilename(const std::string& filename);

		[[nodiscard]] inline static AssetManager& Get() { return *s_instance; }

		template<IsVoltAsset T>
		static Ref<T> GetAsset(AssetHandle assetHandle);

		template<IsVoltAsset T>
		static Ref<T> GetAsset(const std::filesystem::path& path);

		template<IsVoltAsset T>
		static Ref<T> GetAssetLocking(AssetHandle assetHandle);

		template<IsVoltAsset T>
		static Ref<T> GetAssetLocking(const std::filesystem::path& path);

		template<IsVoltAsset T>
		static Ref<T> QueueAsset(AssetHandle handle);

		template<IsVoltAsset T>
		static Ref<T> QueueAsset(const std::filesystem::path& filepath);

		//a memory asset is an asset that can never be saved to disk, it is meant to be only in memory
		template<typename T, typename... Args>
		static Ref<T> CreateMemoryAsset(const std::string& name, Args&&... args);

		//create an asset that doesnt have a path assigned yet
		template<IsVoltAsset T, typename... Args>
		static Ref<T> CreateAsset(const std::string& name, Args&&... args);

		//create an asset and a corresponding file at a path
		template<IsVoltAsset T, typename... Args>
		static Ref<T> CreateAssetAndFile(const std::filesystem::path& targetDir, const std::string& name, Args&&... args);

		//asset must not already have a representing file
		VT_INLINE static void CreateFileForAsset(Volt::AssetHandle asset, const std::filesystem::path& path);

		template<typename ImporterType, typename Type>
		static const ImporterType& GetImporterForType();

		template<IsVoltAsset T>
		static const Vector<Ref<T>> GetAllCachedAssetsOfType();

		template<IsVoltAsset T>
		static const Vector<AssetHandle> GetAllAssetsOfType();

		static const Vector<AssetHandle> GetAllAssetsOfType(AssetType assetType);

	private:
		struct AssetChangedCallbackInfo
		{
			UUID64 id;
			AssetChangedCallback callback;
		};

		struct AssetChangedQueueInfo
		{
			AssetHandle handle;
			AssetChangedState state;
		};

		inline static AssetManager* s_instance = nullptr;
		inline static AssetMetadata s_nullMetadata = {};

		bool UpdateInternal(AppUpdateEvent& event);

		void LoadAsset(AssetHandle assetHandle, Ref<Asset>& asset);

		void LoadAllAssetMetadata();
		void DeserializeAssetMetadata(const std::filesystem::path& assetPath, AssetMetadata& outMetadata);

		void OnAssetChanged(AssetHandle assetHandle, AssetChangedState state);
		void QueueAssetChanged(AssetHandle assetHandle, AssetChangedState state);

		void QueueAssetInternal(AssetHandle assetHandle, Ref<Asset>& asset);

		static bool ValidateAssetType(AssetHandle handle, Ref<Asset> asset);
		static AssetMetadata& GetMetadataFromHandleMutable(AssetHandle handle);
		static AssetMetadata& GetMetadataFromFilePathMutable(const std::filesystem::path filePath);
		static const AssetMetadata& GetMetadataFromHandleLockless(AssetHandle handle);
		static const AssetMetadata& GetMetadataFromFilePathLockless(const std::filesystem::path filePath);
		static const bool HasFilePathLockless(AssetHandle handle);

		static const std::filesystem::path GetCleanAssetFilePath(const std::filesystem::path& path);

		void SaveAssetImpl(AssetHandle handle);
		template<IsVoltAsset T, typename... Args>
		Ref<T> CreateAssetImpl(const std::string& name, bool isMemoryAsset, Args&&... args);
		void CreateFileForAssetImpl(AssetHandle asset, const std::filesystem::path& targetFilePath);

		Vector<std::filesystem::path> GetEngineAssetFiles();
		Vector<std::filesystem::path> GetProjectAssetFiles();

		AssetCache m_assetCache;
		AssetCache m_memoryAssets;
		AssetRegistry m_assetRegistry;

		std::filesystem::path m_projectDirectory;
		std::filesystem::path m_assetsDirectory;
		std::filesystem::path m_engineDirectory;

		std::unordered_map<AssetType, Vector<AssetChangedCallbackInfo>> m_assetChangedCallbacks;
		Vector<AssetChangedQueueInfo> m_assetChangedQueue;
		std::mutex m_assetChangedQueueMutex;
		Scope<AssetDependencyGraph> m_dependencyGraph;

		std::mutex m_assetCallbackMutex;
		mutable std::shared_mutex m_assetRegistryMutex;
		mutable std::shared_mutex m_assetCacheMutex;
	};

	template<IsVoltAsset T>
	inline Ref<T> AssetManager::GetAsset(AssetHandle assetHandle)
	{
		VT_PROFILE_FUNCTION();

		if (assetHandle == Asset::Null())
		{
			return nullptr;
		}

		{
			ReadLock lock{ Get().m_assetRegistryMutex };
			const auto& metadata = GetMetadataFromHandle(assetHandle);
			if (!metadata.IsValid())
			{
				VT_LOGC(Error, LogAssetSystem, "Trying to load asset {} which has invalid metadata!", assetHandle);
				return nullptr;
			}
		}

		Ref<Asset> asset = CreateRef<T>();
		if (!ValidateAssetType(assetHandle, asset))
		{
			VT_LOGC(Critical, LogAssetSystem, "Asset type does not match!");
			return nullptr;
		}

		Get().LoadAsset(assetHandle, asset);
		return std::reinterpret_pointer_cast<T>(asset);
	}

	template<IsVoltAsset T>
	inline Ref<T> AssetManager::GetAsset(const std::filesystem::path& path)
	{
		return GetAsset<T>(GetAssetHandleFromFilePath(path));
	}

	template<IsVoltAsset T>
	inline Ref<T> AssetManager::GetAssetLocking(AssetHandle assetHandle)
	{
		VT_PROFILE_FUNCTION();

		if (assetHandle == Asset::Null())
		{
			return nullptr;
		}

		{
			ReadLock lock{ Get().m_assetRegistryMutex };
			const auto& metadata = GetMetadataFromHandle(assetHandle);
			if (!metadata.IsValid())
			{
				VT_LOGC(Error, LogAssetSystem, "Trying to load asset which has invalid metadata!");
				return nullptr;
			}
		}

		Ref<Asset> asset = QueueAsset<T>(assetHandle);

		if (!asset)
		{
			return nullptr;
		}

		while (!asset->IsValid())
		{
		}
		return std::reinterpret_pointer_cast<T>(asset);
	}

	template<IsVoltAsset T>
	inline Ref<T> AssetManager::GetAssetLocking(const std::filesystem::path& path)
	{
		return GetAssetLocking<T>(GetAssetHandleFromFilePath(path));
	}

	template<IsVoltAsset T>
	inline Ref<T> AssetManager::QueueAsset(AssetHandle handle)
	{
		VT_PROFILE_FUNCTION();

		if (handle == Asset::Null())
		{
			return nullptr;
		}

		const auto metadata = GetMetadataFromHandle(handle);
		if (!metadata.IsValid())
		{
			VT_LOGC(Error, LogAssetSystem, "Trying to load asset which has invalid metadata!");
			return nullptr;
		}

		// If it's a memory asset, return it
		if (Get().m_memoryAssets.contains(handle))
		{
			return std::reinterpret_pointer_cast<T>(Get().m_memoryAssets.at(handle));
		}

		// If it's already loaded, return it
		{
			if (IsLoaded(handle))
			{
				return GetAsset<T>(handle);
			}
		}

		Ref<Asset> asset = CreateRef<T>();
		if (!ValidateAssetType(handle, asset))
		{
			VT_LOGC(Critical, LogAssetSystem, "Asset type does not match!");
			return nullptr;
		}

		asset->SetFlag(AssetFlag::Queued, true);
		Get().QueueAssetInternal(handle, asset);

		return std::reinterpret_pointer_cast<T>(asset);
	}

	template<IsVoltAsset T>
	inline Ref<T> AssetManager::QueueAsset(const std::filesystem::path& filepath)
	{
		return QueueAsset<T>(GetAssetHandleFromFilePath(filepath));
	}

	template<typename T, typename ...Args>
	inline Ref<T> AssetManager::CreateMemoryAsset(const std::string& name, Args&& ...args)
	{
		return Get().CreateAssetImpl<T>(name, /*isMemoryAsset*/ true, std::forward<Args>(args)...);
	}

	template<IsVoltAsset T, typename ...Args>
	inline Ref<T> AssetManager::CreateAsset(const std::string& name, Args && ...args)
	{
		return Get().CreateAssetImpl<T>(name, /*isMemoryAsset*/ false, std::forward<Args>(args)...);
	}

	template<IsVoltAsset T, typename ...Args>
	inline Ref<T> AssetManager::CreateAssetImpl(const std::string& name, bool isMemoryAsset, Args && ...args)
	{
		Ref<T> asset = CreateRef<T>(std::forward<Args>(args)...);

		//#TODO_Ivar: Move to clean name function
		std::string cleanName = name;
		cleanName.erase(std::remove_if(cleanName.begin(), cleanName.end(), [](char c) { return c == ':'; }), cleanName.end());

		AssetMetadata metadata{};
		metadata.filePath = ""; // assets that are not saved will not have a file path
		metadata.handle = asset->handle; // handle will have generated on asset creation
		metadata.type = T::GetStaticType();
		metadata.isLoaded = true;
		metadata.isMemoryAsset = isMemoryAsset;

		asset->SetupInitialCustomMetadata(metadata.customData);

		asset->assetName = cleanName;

		{
			WriteLock lockCache{ m_assetCacheMutex };
			WriteLock lockRegistry{ m_assetRegistryMutex };

			m_assetRegistry.emplace(asset->handle, metadata);
			if (isMemoryAsset)
			{
				m_memoryAssets.emplace(asset->handle, asset);
			}
			else
			{
				m_assetCache.emplace(asset->handle, asset);
			}
		}

		AssetCreatedEvent assetCreatedEvent(asset->handle);
		EventSystem::DispatchEvent(assetCreatedEvent);
		return asset;
	}

	template<IsVoltAsset T, typename ...Args>
	inline Ref<T> AssetManager::CreateAssetAndFile(const std::filesystem::path& targetDir, const std::string& name, Args && ...args)
	{
		Ref<T> asset = CreateAsset<T>(name, std::forward<Args>(args)...);

		CreateFileForAsset(asset->handle, targetDir);

		return asset;
	}

	void AssetManager::CreateFileForAsset(Volt::AssetHandle asset, const std::filesystem::path& path)
	{
		Get().CreateFileForAssetImpl(asset, path);
	}

	template<typename ImporterType, typename Type>
	inline const ImporterType& AssetManager::GetImporterForType()
	{
		const auto type = Type::GetStaticType();
		VT_ASSERT_MSG(Get().m_assetSerializers.contains(type), "Importer for type does not exist!");

		return (ImporterType&)*Get().m_assetSerializers.at(type);
	}
	template<IsVoltAsset T>
	inline const Vector<Ref<T>> AssetManager::GetAllCachedAssetsOfType()
	{
		ReadLock lock{ Get().m_assetCacheMutex };

		Vector<Ref<T>> result{};

		for (const auto& [handle, asset] : Get().m_assetCache)
		{
			if (asset->GetType() == T::GetStaticType())
			{
				result.push_back(reinterpret_pointer_cast<T>(asset));
			}
		}

		return result;
	}

	template<IsVoltAsset T>
	inline const Vector<AssetHandle> AssetManager::GetAllAssetsOfType()
	{
		return GetAllAssetsOfType(T::GetStaticType());
	}

}
