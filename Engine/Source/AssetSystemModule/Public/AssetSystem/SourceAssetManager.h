#pragma once

#include "AssetSystem/Config.h"

#include "AssetSystem/SourceAssetImporterRegistry.h"
#include "AssetSystem/SourceAssetImporter.h"
#include "AssetSystem/AssetManager.h"
#include "AssetSystem/SourceAssetImportConfig.h"

#include <JobSystem/JobPromise.h>
#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/WorkQueue.h>
#include <CoreUtilities/Pointers/Unique.h>

VT_DECLARE_LOG_CATEGORY_EXPORT(VTAS_API, LogSourceAssetManager, LogVerbosity::Trace);

namespace Volt
{
	class Asset;
	class VTAS_API SourceAssetManager
	{
	public:
		using ImportedCallbackFunc = std::function<void(Vector<AssetReference<Asset>>)>;

		SourceAssetManager();
		~SourceAssetManager();

		template<typename ConfigType>
		static JobFuture<Vector<AssetReference<Asset>>> ImportSourceAsset(const Filesystem::Path& filepath, const ConfigType& config, const SourceAssetUserImportData& userData = {})
		{
			static_assert(std::is_base_of_v<SourceAssetImportConfig, ConfigType>);

			VT_ENSURE(s_instance);
			VT_ENSURE(!filepath.IsEmpty());

			auto importFunc = [=]() -> Vector<AssetReference<Asset>>
			{
				const String extension = filepath.Extension().ToString();
				auto& importer = SourceAssetImporterRegistry::Get().GetImporterForExtension(extension);
				return importer.Import(g_assetManager->GetAssetFilesystemPath(filepath), config, userData);
			};

			return s_instance->ImportSourceAssetInternal(std::move(importFunc), config, filepath);
		}

		template<typename ConfigType>
		static void ImportSourceAsset(const Filesystem::Path& filepath, const ConfigType& config, const ImportedCallbackFunc& importedCallback, const SourceAssetUserImportData& userData = {})
		{
			static_assert(std::is_base_of_v<SourceAssetImportConfig, ConfigType>);

			VT_ENSURE(s_instance);
			VT_ENSURE(importedCallback);
			VT_ENSURE(!filepath.IsEmpty());

			auto importFunc = [=]() -> Vector<AssetReference<Asset>>
			{
				const String extension = filepath.Extension().ToString();
				auto& importer = SourceAssetImporterRegistry::Get().GetImporterForExtension(extension);
				return importer.Import(g_assetManager->GetAssetFilesystemPath(filepath), config, userData);
			};

			s_instance->ImportSourceAssetInternal(std::move(importFunc), importedCallback, config, filepath);
		}

		static SourceAssetFileInformation GetSourceAssetFileInformation(const Filesystem::Path& filepath);

	private:
		using ImportJobFunc = std::function<Vector<AssetReference<Asset>>()>;

		struct ImportJob
		{
			JobRef job = nullptr;
			Ref<JobPromise<Vector<AssetReference<Asset>>>> resultPromise;
			String debugString;
		};

		JobFuture<Vector<AssetReference<Asset>>> ImportSourceAssetInternal(ImportJobFunc&& importFunc, const SourceAssetImportConfig& importConfig, const Filesystem::Path& filepath);
		void ImportSourceAssetInternal(ImportJobFunc&& importFunc, const ImportedCallbackFunc& importedCallback, const SourceAssetImportConfig& importConfig, const Filesystem::Path& filepath);

		void RunAssetImportWorker();

		inline static SourceAssetManager* s_instance = nullptr;

		std::atomic_bool m_isRunning = true;
		std::mutex m_wakeMutex;
		std::condition_variable m_wakeCondition;
		Unique<std::thread> m_assetImporterWorkerThread;

		WorkQueue<ImportJob, QueueThreadingPolicy::MPSC> m_importQueue;
	};
}
