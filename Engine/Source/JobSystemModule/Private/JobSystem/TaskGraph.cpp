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

		// Compile the graph, this fills the m_jobs member.
		Compile();
		
		JobSystem::RunJobs(m_jobs);

		m_isExecuted = true;

		VT_ENSURE(m_jobs.size() == m_tasks.size());
	}

	JobCounterRef TaskGraph::ExecuteAndExtractCounter()
	{
		VT_PROFILE_FUNCTION();

		Execute();
		m_graphCounter->IncRef();
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
		VT_ENSURE_MSG(m_isExecuted, "Waiting on a graph without executing it will cause an eternal wait!");
		JobSystem::WaitForCounter(m_graphCounter);
	}

	void TaskGraph::Compile()
	{
		VT_PROFILE_FUNCTION();

		struct StackEntry
		{
			JobRef dependantJob = nullptr;
			Task* task = nullptr;
		};

		// Create the graphs counter that is waitable.
		m_graphCounter = JobSystem::CreateCounter();

		// Find all unreferenced tasks, aka all tasks
		// that no other task depends on.
		Vector<Task*> unreferencedTasks;
		for (Task* task : m_tasks)
		{
			if (task->GetRefCount() == 0)
			{
				unreferencedTasks.emplace_back(task);
			}
		}

		m_jobs.reserve(m_numExpectedTasks);

		// Now iterate through all unreferenced tasks and 
		// iterate though their dependency trees, creating
		// the jobs as we go.
		for (Task* unreferencedTask : unreferencedTasks)
		{
			JobRef initialJob = unreferencedTask->CreateJob(m_priority, m_graphCounter);
			m_jobs.emplace_back(initialJob);

			Vector<StackEntry> dependencyStack;
			dependencyStack.reserve(unreferencedTask->GetDependencies().size());

			for (Task* initialDependency : unreferencedTask->GetDependencies())
			{
				auto& newEntry = dependencyStack.emplace_back();
				newEntry.task = initialDependency;
				newEntry.dependantJob = initialJob;
			}

			while (!dependencyStack.empty())
			{
				StackEntry currentEntry = dependencyStack.back();
				dependencyStack.pop_back();

				JobRef job = currentEntry.task->CreateJobAsDependency(m_priority, currentEntry.dependantJob);
				m_jobs.emplace_back(job);

				for (Task* dependency : currentEntry.task->GetDependencies())
				{
					auto& newEntry = dependencyStack.emplace_back();
					newEntry.task = dependency;
					newEntry.dependantJob = job;
				}
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
