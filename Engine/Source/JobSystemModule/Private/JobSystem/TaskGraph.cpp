#include "jspch.h"
#include "TaskGraph.h"

namespace Volt
{
	TaskGraph::TaskGraph(size_t predictedTaskCount)
		: m_predictedTaskCount(predictedTaskCount)
	{
		m_createdJobs.reserve(predictedTaskCount);
		m_graphCounter = JobSystem::CreateCounter();
	}

	TaskGraph::~TaskGraph()
	{
		JobSystem::DestroyCounter(m_graphCounter);
	}

	void TaskGraph::Execute()
	{
		m_executed = true;
		JobSystem::RunJobs(m_createdJobs);
	}

	void TaskGraph::ExecuteAndWait()
	{
		Execute();
		JobSystem::WaitForCounter(m_graphCounter);
	}

	void TaskGraph::Wait()
	{
		VT_ENSURE(m_executed);
		JobSystem::WaitForCounter(m_graphCounter);
	}
}
