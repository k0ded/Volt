#include "aspch.h"

#include "AssetSystem/SourceAssetImporter.h"
#include "AssetSystem/AssetLocks.h"

#include "SourceAssetManager.h"

#include <Volt-Platforms/Platform.h>

#include <CoreUtilities/Profiling/Profiling.h>

VT_DEFINE_LOG_CATEGORY(LogSourceAssetManager);

namespace Volt
{
	inline std::filesystem::path GetNonExistingFilePath(const std::filesystem::path& directory, std::string_view filename)
	{
		std::string filenameStr = std::string(filename);

		std::filesystem::path filePath = g_assetManager->GetFilesystemPath(directory / (filenameStr + ".vtasset"));
		uint32_t counter = 0;

		while (std::filesystem::exists(filePath))
		{
			filePath = g_assetManager->GetFilesystemPath(directory / (filenameStr + "_" + std::to_string(counter) + ".vtasset"));
			counter++;
		}

		return g_assetManager->GetRelativeAssetFilepath(filePath);
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

	JobFuture<Vector<AssetReference<Asset>>> SourceAssetManager::ImportSourceAssetInternal(ImportJobFunc&& importFunc, const SourceAssetImportConfig& importConfig, const std::filesystem::path& filepath)
	{
		const std::string extension = filepath.extension().string();

		if (!GetSourceAssetImporterRegistry().ImporterForExtensionExists(extension))
		{
			VT_LOGC(Warning, LogSourceAssetManager, "Trying to import and asset but no importer for the extension {} exists!", extension);
			return {};
		}

		auto resultPromise = CreateRef<JobPromise<Vector<AssetReference<Asset>>>>();

		// Create a counter which we supply to the promise.
		JobCounterRef importCounter = JobSystem::CreateCounter();
		JobRef importJobRef = JobSystem::CreateJob("Import Source Asset", ExecutionPriority::Latent, importCounter, [this, extension, importFunc, resultPromise, importConfig]()
		{
			auto result = importFunc();

			if (importConfig.createAsMemoryAsset)
			{
				for (const auto& asset : result)
				{
					ScopedAssetReferenceLock assetLock{ asset };

					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} (Handle: {}) was imported!", asset->GetAssetName(), asset->GetAssetHandle());
				}
			}
			else
			{
				for (const auto& asset : result)
				{
					ScopedAssetReferenceLock assetLock{ asset };

					std::filesystem::path filepath = GetNonExistingFilePath(importConfig.destinationDirectory, std::string(asset->GetAssetName()));
					g_assetManager->CreateFileForAsset(asset->GetAssetHandle(), filepath);

					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} was imported and saved to {}", asset->GetAssetName(), filepath);
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
		importQueue.Emplace(importJob);

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

		JobRef importJobRef = JobSystem::CreateJob("Import Source Asset", ExecutionPriority::Latent, [this, extension, importFunc, importedCallback, importConfig]()
		{
			auto result = importFunc();

			if (importConfig.createAsMemoryAsset)
			{
				for (const auto asset : result)
				{
					ScopedAssetReferenceLock assetLock{ asset };

					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} (Handle: {}) was imported!", asset->GetAssetName(), asset->GetAssetHandle());
				}
			}
			else
			{
				for (const auto asset : result)
				{
					ScopedAssetReferenceLock assetLock{ asset };

					std::filesystem::path filepath = GetNonExistingFilePath(importConfig.destinationDirectory, std::string(asset->GetAssetName()));
					g_assetManager->CreateFileForAsset(asset->GetAssetHandle(), filepath);

					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} (Handle: {}) was imported and saved to {}", asset->GetAssetName(), asset->GetAssetHandle(), filepath);
				}
			}

			*m_isImporterInUseMap[extension] = false;
			m_wakeCondition.notify_one();

			JobRef callbackJob = JobSystem::CreateJob("Import Callback", ExecutionPriority::Latent, ExecutionPolicy::MainThread, [importedCallback, result]()
			{
				importedCallback(result);
			});
			JobSystem::RunJob(callbackJob);
		});

		ImportJob importJob;
		importJob.job = importJobRef;
		importJob.debugString = filepath.string();

		auto& importQueue = GetOrCreateQueue(extension);
		importQueue.Emplace(importJob);

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

		return GetSourceAssetImporterRegistry().GetImporterForExtension(extension).GetSourceFileInformation(g_assetManager->GetFilesystemPath(filepath));
	}

	WorkQueue<SourceAssetManager::ImportJob, QueueThreadingPolicy::MPSC>& SourceAssetManager::GetOrCreateQueue(const std::string& extension)
	{
		if (m_importQueues.contains(extension))
		{
			return *m_importQueues.at(extension);
		}

		constexpr uint32_t NumMaxImportJobs = 2048;

		m_importQueues[extension] = CreateScope<WorkQueue<ImportJob, QueueThreadingPolicy::MPSC>>();
		m_importQueues[extension]->Allocate(NumMaxImportJobs);

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
				if (queue->Pop(jobHolder))
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
