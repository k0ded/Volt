#include "vtassetspch.h"

#include "Volt-Assets/MaterialCompilerSubSystem.h"
#include "Volt-Assets/MaterialCompiler.h"

#include <CoreUtilities/ThreadUtilities.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(MaterialCompilerSubSystem, PostEngine, 0);

	void MaterialCompilerSubSystem::Initialize()
	{
		m_workerThread = CreateScope<std::thread>(std::bind(&MaterialCompilerSubSystem::RunWorker, this));
		Thread::SetThreadName(m_workerThread->native_handle(), "MaterialCompilerWorker");
		Thread::SetThreadPriority(m_workerThread->native_handle(), ThreadPriority::Low);
	}

	void MaterialCompilerSubSystem::Shutdown()
	{
		m_isRunning = false;
		m_wakeCondition.notify_all();
		m_workerThread->join();
		m_workerThread = nullptr;
	}

	void MaterialCompilerSubSystem::RequestMaterialCompilation(Ref<MaterialAsset> materialAsset)
	{
		CompilationJob job;
		job.material = materialAsset;
		m_queue->push(job);
	}

	void MaterialCompilerSubSystem::RunWorker()
	{
		while (m_isRunning)
		{
			CompilationJob job;
			while (m_queue->try_pop(job))
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
