#include "aspch.h"

#include "AssetSystem/SourceAssetImporter.h"

#include "SourceAssetManager.h"

#include <FileSystemModule/Filesystem.h>
#include <Volt-Platforms/Platform.h>

#include <CoreUtilities/Profiling/Profiling.h>

VT_DEFINE_LOG_CATEGORY(LogSourceAssetManager);

namespace Volt
{
	inline Filesystem::Path GetNonExistingFilePath(const Filesystem::Path& directory, StringView filename)
	{
		String filenameStr = String(filename);

		Filesystem::Path filePath = g_assetManager->GetAssetFilesystemPath(directory / (filenameStr + ".vtasset"));
		uint32_t counter = 0;

		while (Filesystem::Exists(filePath))
		{
			filePath = g_assetManager->GetAssetFilesystemPath(directory / (FormatString("{}_{}.vtasset", filenameStr, counter)));
			counter++;
		}

		return g_assetManager->GetRelativeAssetFilepath(filePath);
	}

	SourceAssetManager::SourceAssetManager()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;

		m_importQueue.Allocate(2048);

		m_assetImporterWorkerThread = CreateUnique<std::thread>(std::bind(&SourceAssetManager::RunAssetImportWorker, this));
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

	JobFuture<Vector<AssetReference<Asset>>> SourceAssetManager::ImportSourceAssetInternal(ImportJobFunc&& importFunc, const SourceAssetImportConfig& importConfig, const Filesystem::Path& filepath)
	{
		const String extension = filepath.Extension().ToString();

		if (!SourceAssetImporterRegistry::Get().ImporterForExtensionExists(extension))
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
					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} (Handle: {}) was imported!", asset->GetAssetName(), asset->GetAssetHandle());
				}
			}
			else
			{
				for (const auto& asset : result)
				{
					Filesystem::Path filepath = GetNonExistingFilePath(importConfig.destinationDirectory, String(asset->GetAssetName()));
					g_assetManager->CreateFileForAsset(asset->GetAssetHandle(), filepath);

					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} was imported and saved to {}", asset->GetAssetName(), filepath);
				}
			}

			resultPromise->SetValue(result);
		}, FiberStackSize::KB128);

		resultPromise->SetAssociatedCounter(importCounter);

		ImportJob importJob;
		importJob.resultPromise = resultPromise;
		importJob.job = importJobRef;
		importJob.debugString = filepath.ToString();

		m_importQueue.Emplace(importJob);
		m_wakeCondition.notify_all();

		return resultPromise->GetFuture();
	}

	void SourceAssetManager::ImportSourceAssetInternal(ImportJobFunc&& importFunc, const ImportedCallbackFunc& importedCallback, const SourceAssetImportConfig& importConfig, const Filesystem::Path& filepath)
	{
		const String extension = filepath.Extension().ToString();

		if (!SourceAssetImporterRegistry::Get().ImporterForExtensionExists(extension))
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
					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} (Handle: {}) was imported!", asset->GetAssetName(), asset->GetAssetHandle());
				}
			}
			else
			{
				for (const auto asset : result)
				{
					Filesystem::Path filepath = GetNonExistingFilePath(importConfig.destinationDirectory, String(asset->GetAssetName()));
					g_assetManager->CreateFileForAsset(asset->GetAssetHandle(), filepath);

					VT_LOGC(Trace, LogSourceAssetManager, "Asset {} (Handle: {}) was imported and saved to {}", asset->GetAssetName(), asset->GetAssetHandle(), filepath);
				}
			}

			JobRef callbackJob = JobSystem::CreateJob("Import Callback", ExecutionPriority::Latent, ExecutionPolicy::MainThread, [importedCallback, result]()
			{
				importedCallback(result);
			});
			JobSystem::RunJob(callbackJob);
		});

		ImportJob importJob;
		importJob.job = importJobRef;
		importJob.debugString = filepath.ToString();

		m_importQueue.Emplace(importJob);
		m_wakeCondition.notify_all();
	}

	SourceAssetFileInformation SourceAssetManager::GetSourceAssetFileInformation(const Filesystem::Path& filepath)
	{
		VT_ENSURE(s_instance);

		const String extension = filepath.Extension().ToString();

		if (!SourceAssetImporterRegistry::Get().ImporterForExtensionExists(extension))
		{
			VT_LOGC(Warning, LogSourceAssetManager, "Trying to get file information of asset but no importer for the extension {} exists!", extension);
			return {};
		}

		return SourceAssetImporterRegistry::Get().GetImporterForExtension(extension).GetSourceFileInformation(g_assetManager->GetAssetFilesystemPath(filepath));
	}

	void SourceAssetManager::RunAssetImportWorker()
	{
		while (m_isRunning)
		{
			ImportJob jobHolder;
			while (m_importQueue.Pop(jobHolder))
			{
				JobSystem::RunJob(jobHolder.job);
				VT_LOGC(Trace, LogSourceAssetManager, "Dispatched Import job for '{}'", jobHolder.debugString);
			}

			std::unique_lock lock{ m_wakeMutex };
			m_wakeCondition.wait(lock);
		}
	}
}
