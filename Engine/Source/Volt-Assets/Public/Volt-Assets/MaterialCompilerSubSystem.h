#pragma once

#include "Volt-Assets/Config.h"

#include <SubSystem/SubSystem.h>

#include <CoreUtilities/WorkQueue.h>

namespace Volt
{
	class MaterialAsset;

	class VTASSETS_API MaterialCompilerSubSystem : public SubSystem
	{ 
	public:
		void Initialize() override;
		void Shutdown() override;

		void RequestMaterialCompilation(Ref<MaterialAsset> materialAsset);

		VT_DECLARE_SUBSYSTEM("{EEB3C410-3128-486A-8F66-810CEB39314C}"_guid);
	private:
		struct CompilationJob
		{
			Ref<MaterialAsset> material;
		};
		
		void RunWorker();
		void ExecuteJob(const CompilationJob& job);

		std::atomic_bool m_isRunning = true;
		std::condition_variable m_wakeCondition;
		std::mutex m_wakeMutex;
		
		Scope<std::thread> m_workerThread;
		WorkQueue<CompilationJob, QueueThreadingPolicy::MPSC> m_queue;
	};
}
