#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/JobSystem.h"

namespace Volt
{
	class VTJS_API TaskGraph
	{
	public:
		TaskGraph(size_t predictedTaskCount = 100);
		~TaskGraph();

		VT_DELETE_COPY_MOVE(TaskGraph);

		template<typename Func>
		Job* AddTask(std::string_view name, ExecutionPriority priority, Func&& func);

		void Execute();
		void ExecuteAndWait();
		void Wait();

	private:
		Vector<Job*> m_createdJobs;
		
		JobCounter* m_graphCounter = nullptr;
		size_t m_predictedTaskCount;
		bool m_executed = false;
	};

	template<typename Func>
	Job* TaskGraph::AddTask(std::string_view name, ExecutionPriority priority, Func&& func)
	{
		Job* job = JobSystem::CreateJob(name, priority, m_graphCounter, std::move(func));
		m_createdJobs.emplace_back(job);

		return job;
	}
}
