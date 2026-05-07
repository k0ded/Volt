#include "jspch.h"
#include "TaskGraph.h"

#include <CoreUtilities/Malloc.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	TaskGraph::TaskGraph(ExecutionPriority priority, size_t numExpectedTasks)
		: m_priority(priority),
		m_numExpectedTasks(numExpectedTasks)
	{
		VT_UNUSED(m_numExpectedTasks);
	}

	TaskGraph::~TaskGraph()
	{
		if (m_graphCounter)
		{
			JobSystem::DestroyCounter(m_graphCounter);
		}
	}

	void TaskGraph::Execute()
	{
		VT_PROFILE_FUNCTION();

		if (m_tasks.empty())
		{
			return;
		}

		// Compile the graph, this fills the m_jobs member.
		Compile();
		JobSystem::RunJobs(m_jobs);

		m_isExecuted = true;

	}

	JobCounterRef TaskGraph::ExecuteAndExtractCounter()
	{
		VT_PROFILE_FUNCTION();

		Execute();

		if (m_graphCounter)
		{
			m_graphCounter->IncRef();
		}

		return m_graphCounter;
	}

	void TaskGraph::ExecuteAndWait()
	{
		VT_PROFILE_FUNCTION();

		Execute();
		Wait();
	}

	void TaskGraph::Wait()
	{
		if (!m_graphCounter)
		{
			return;
		}

		VT_ENSURE_MSG(m_isExecuted, "Waiting on a graph without executing it will cause an eternal wait!");
		JobSystem::WaitForCounter(m_graphCounter);
	}

	void TaskGraph::Compile()
	{
		VT_PROFILE_FUNCTION();

		m_graphCounter = JobSystem::CreateCounter();

		GlobalMemoryStackMark memMark;
		std::unordered_set<Task*> visited;

		GlobalMemoryStackVector<Task*> sortedTasks;
		sortedTasks.reserve(m_tasks.size());

		// DFS order
		auto insertFunc = [&](Task* task, auto& insertFunc)
		{
			if (!visited.insert(task).second)
			{
				return;
			}

			for (Task* dependency : task->GetDependencies())
			{
				insertFunc(dependency, insertFunc);
			}

			sortedTasks.emplace_back(task);
		};

		for (Task* task : m_tasks)
		{
			insertFunc(task, insertFunc);
		}

		Map<Task*, size_t> taskToIndex;
		taskToIndex.reserve(sortedTasks.size());

		// Create all jobs without any counters
		for (size_t i = 0; i < sortedTasks.size(); ++i)
		{
			Task* task = sortedTasks[i];
			m_jobs.emplace_back(task->CreateJob(m_priority));
			taskToIndex[task] = i;
		}

		// Build in topological order
		// Note: index in sortedTasks matches job in m_jobs
		for (size_t i = 0; i < sortedTasks.size(); ++i)
		{
			Task* task = sortedTasks[i];
			JobRef job = m_jobs[i];

			JobCounterRef waitCounter = JobSystem::CreateCounter();
			job->SetWaitCounter(waitCounter);

			// Add wait counter as an associated counter to dependencies.
			for (Task* dependency : task->GetDependencies())
			{
				const size_t dependencyIndex = taskToIndex[dependency];
				JobRef dependencyJob = m_jobs[dependencyIndex];

				dependencyJob->AddAssociatedCounter(waitCounter);
			}

			// Root job, add graph counter as associated counter
			if (task->GetRefCount() == 0)
			{
				job->AddAssociatedCounter(m_graphCounter);
			}
		}
	}

	TaskGraphAllocator::~TaskGraphAllocator()
	{
		for (auto& destructor : m_taskDestructors)
		{
			destructor.destructor(destructor.dataPtr);
		}
	}

	void* TaskGraphAllocator::AllocateBytes(size_t size)
	{
		return m_allocator.Allocate(size);
	}

	void TaskGraph::Task::AddDependency(Task* dependency)
	{
		dependency->IncRef();

		m_dependencies.emplace_back(dependency);
	}
	
	void TaskGraph::Task::AddDependencies(std::span<Task*> dependencies)
	{
		for (Task* dependency : dependencies)
		{
			dependency->IncRef();
		}

		m_dependencies.append(dependencies.begin(), dependencies.end());
	}

	void TaskGraph::Task::AddDependencies(std::initializer_list<Task*> dependencies)
	{
		for (Task* dependency : dependencies)
		{
			dependency->IncRef();
		}

		m_dependencies.append(dependencies.begin(), dependencies.end());
	}
}
