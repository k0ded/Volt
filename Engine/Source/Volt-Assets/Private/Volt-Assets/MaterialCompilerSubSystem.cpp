#include "vtassetspch.h"

#include "Volt-Assets/MaterialCompilerSubSystem.h"
#include "Volt-Assets/MaterialCompiler.h"

#include <Volt-Platforms/Platform.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(MaterialCompilerSubSystem, Default, PostEngine, 0);

	void MaterialCompilerSubSystem::Initialize()
	{
		m_queue.Allocate(4096);

		m_workerThread = CreateScope<std::thread>(std::bind(&MaterialCompilerSubSystem::RunWorker, this));
		PlatformThread::SetThreadName(m_workerThread->native_handle(), "MaterialCompilerWorker");
		PlatformThread::SetThreadPriority(m_workerThread->native_handle(), ThreadPriority::Low);
	}

	void MaterialCompilerSubSystem::Shutdown()
	{
		m_isRunning = false;
		m_wakeCondition.notify_all();
		m_workerThread->join();
		m_workerThread = nullptr;
	}

	void MaterialCompilerSubSystem::RequestMaterialCompilation(AssetReference<MaterialAsset> materialAsset)
	{
		CompilationJob job;
		job.material = materialAsset;
		m_queue.Emplace(job);
		m_wakeCondition.notify_all();
	}

	void MaterialCompilerSubSystem::RunWorker()
	{
		while (m_isRunning)
		{
			CompilationJob job;
			while (m_queue.Pop(job))
			{
				ExecuteJob(job);
			}

			std::unique_lock lock(m_wakeMutex);
			m_wakeCondition.wait(lock);
		}
	}

	void MaterialCompilerSubSystem::ExecuteJob(const CompilationJob& job)
	{
		MaterialCompiler compiler;
		compiler.CompileMaterial(job.material);
	}
}
