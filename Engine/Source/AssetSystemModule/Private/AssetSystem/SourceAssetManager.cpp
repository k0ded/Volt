#include "aspch.h"

#include "AssetSystem/SourceAssetImporter.h"
#include "SourceAssetManager.h"

#include <Volt-Platforms/Platform.h>

#include <CoreUtilities/Profiling/Profiling.h>

VT_DEFINE_LOG_CATEGORY(LogSourceAssetManager);

namespace Volt
{
	inline std::filesystem::path GetNonExistingFilePath(const std::filesystem::path& directory, const std::string& fileName)
	{
		std::filesystem::path filePath = AssetManager::GetFilesystemPath(directory / (fileName + ".vtasset"));
		uint32_t counter = 0;

		while (std::filesystem::exists(filePath))
		{
			filePath = AssetManager::GetFilesystemPath(directory / (fileName + "_" + std::to_string(counter) + ".vtasset"));
			counter++;
		}

		return AssetManager::GetRelativePath(filePath);
	}

	SourceAssetManager::SourceAssetManager()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;

		m_assetImporterWorkerThread = CreateScope<std::thread>(std::bind(&SourceAssetManager::RunAssetImportWorker, this));
		PlatformThread::SetThreadName(m_assetImporterWorkerThread->native_handle(), "AssetImporterWorker");
		PlatformThread::SetThreadPriority(m_assetImporterWorkerThread->native_handle(), ThreadPriority::Low);
	}

	SourceAssetManager::~SourceAssetManager()
	{
		m_isRunning = false;
		m_wakeCondition.notify_all();

		m_assetImporterWorkerThread->join();

		s_instance = nullptr;
	}

	JobFuture<Vector<Ref<Asset>>> SourceAssetManager::ImportSourceAssetInternal(ImportJobFunc&& importFunc, const SourceAssetImportConfig& importConfig, const std::filesystem::path& filepath)
	{
		const std::string extension = filepath.extension().string();

		if (!GetSourceAssetImporterRegistry().ImporterForExtensionExists(extension))
		{
			VT_LOGC(Warning, LogSourceAssetManager, "Trying to import and asset but no importer for the extension {} exists!", extension);
			return {};
		}

		auto resultPromise = CreateRef<JobPromise<Vector<Ref<Asset>>>>();

		// Create a counter which we supply to the promise.
		JobCounterRef importCounter = JobSystem::CreateCounter();
		JobRef importJobRef = JobSystem::CreateJob("Import Source Asset", importCounter, [this, extension, importFunc, resultPromise, importConfig]()
		{
			auto result = importFunc();

			if (importConfig.createAsMemoryAsset)
			{
				for (const auto asset : result)
				{
					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} with handle {} was imported!", asset->assetName, asset->handle);
				}
			}
			else
			{
				for (const auto asset : result)
				{
					std::filesystem::path filePath = AssetManager::GetFilePathFromAssetHandle(asset->handle);
					filePath = GetNonExistingFilePath(filePath.parent_path(), filePath.stem().string());
					AssetManager::SaveAssetAs(asset, filePath);

					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} was imported and saved to {}", asset->assetName, filePath);
				}
			}

			resultPromise->SetValue(result);

			*m_isImporterInUseMap[extension] = false;
			m_wakeCondition.notify_one();
		});

		resultPromise->SetAssociatedCounter(importCounter);

		ImportJob importJob;
		importJob.resultPromise = resultPromise;
		importJob.job = importJobRef;
		importJob.debugString = filepath.string();

		auto& importQueue = GetOrCreateQueue(extension);
		importQueue.push(importJob);

		m_wakeCondition.notify_one();

		return resultPromise->GetFuture();
	}

	void SourceAssetManager::ImportSourceAssetInternal(ImportJobFunc&& importFunc, const ImportedCallbackFunc& importedCallback, const SourceAssetImportConfig& importConfig, const std::filesystem::path& filepath)
	{
		const std::string extension = filepath.extension().string();

		if (!GetSourceAssetImporterRegistry().ImporterForExtensionExists(extension))
		{
			VT_LOGC(Warning, LogSourceAssetManager, "Trying to import and asset but no importer for the extension {} exists!", extension);
			return;
		}

		JobRef importJobRef = JobSystem::CreateJob("Import Source Asset", [this, extension, importFunc, importedCallback, importConfig]()
		{
			auto result = importFunc();

			if (importConfig.createAsMemoryAsset)
			{
				for (const auto asset : result)
				{
					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} with handle {} was imported!", asset->assetName, asset->handle);
				}
			}
			else
			{
				for (const auto asset : result)
				{
					std::filesystem::path filePath = AssetManager::GetFilePathFromAssetHandle(asset->handle);
					filePath = GetNonExistingFilePath(filePath.parent_path(), filePath.stem().string());
					AssetManager::SaveAssetAs(asset, filePath);

					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} was imported and saved to {}", asset->assetName, filePath);
				}
			}

			*m_isImporterInUseMap[extension] = false;
			m_wakeCondition.notify_one();

			JobRef callbackJob = JobSystem::CreateJob("Import Callback", ExecutionPolicy::MainThread, [importedCallback, result]() 
			{
				importedCallback(result);
			});
			JobSystem::RunJob(callbackJob);
		});

		ImportJob importJob;
		importJob.job = importJobRef;
		importJob.debugString = filepath.string();

		auto& importQueue = GetOrCreateQueue(extension);
		importQueue.push(importJob);

		m_wakeCondition.notify_one();
	}

	SourceAssetFileInformation SourceAssetManager::GetSourceAssetFileInformation(const std::filesystem::path& filepath)
	{
		VT_ENSURE(s_instance);

		const std::string extension = filepath.extension().string();

		if (!GetSourceAssetImporterRegistry().ImporterForExtensionExists(extension))
		{
			VT_LOGC(Warning, LogSourceAssetManager, "Trying to get file information of asset but no importer for the extension {} exists!", extension);
			return {};
		}

		return GetSourceAssetImporterRegistry().GetImporterForExtension(extension).GetSourceFileInformation(AssetManager::GetFilesystemPath(filepath));
	}

	ThreadSafeQueue<SourceAssetManager::ImportJob>& SourceAssetManager::GetOrCreateQueue(const std::string& extension)
	{
		if (m_importQueues.contains(extension))
		{
			return *m_importQueues.at(extension);
		}

		m_importQueues[extension] = CreateScope<ThreadSafeQueue<ImportJob>>();
		return *m_importQueues.at(extension);
	}

	void SourceAssetManager::RunAssetImportWorker()
	{
		while (m_isRunning)
		{
			for (auto& [ext, queue] : m_importQueues)
			{
				if (!m_isImporterInUseMap.contains(ext))
				{
					m_isImporterInUseMap[ext] = CreateScope<std::atomic_bool>();
					*m_isImporterInUseMap[ext] = false;
				}

				if (*m_isImporterInUseMap[ext])
				{
					continue;
				}

				ImportJob jobHolder;
				if (queue->try_pop(jobHolder))
				{
					*m_isImporterInUseMap[ext] = true;
					JobSystem::RunJob(jobHolder.job);
				}
			}

			std::unique_lock lock{ m_wakeMutex };
			m_wakeCondition.wait(lock);
		}
	}
}
