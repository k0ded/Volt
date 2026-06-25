#include "jspch.h"
#include "TaskGraph.h"

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

#if VT_DEBUG
		ValidateDependencyChains();
#endif

		m_graphCounter = JobSystem::CreateCounter();

		GlobalMemoryStackMark memMark;

		AtomicBitVector<uint64_t, GlobalMemoryStackAllocator> visitedBitmask;
		GlobalMemoryStackVector<Task*> sortedTasks;

		sortedTasks.reserve(m_tasks.size());
		visitedBitmask.Resize(m_tasks.size());

		// DFS sort
		{
			GlobalMemoryStackVector<Task*> stack;
			stack.reserve(m_tasks.size());

			for (size_t i = 0; i < m_tasks.size(); ++i)
			{
				stack.emplace_back(m_tasks[i]);
			}

			while (!stack.empty())
			{
				Task* currTask = stack.back();
				stack.pop_back();

				if (visitedBitmask.Test(currTask->m_index, std::memory_order::relaxed))
				{
					continue;
				}

				for (Task* dependency : currTask->GetDependencies())
				{
					stack.emplace_back(dependency);
				}

				sortedTasks.emplace_back(currTask);
				visitedBitmask.SetBit(currTask->m_index, true, std::memory_order::relaxed);
			}
		}

		for (size_t i = 0; i < sortedTasks.size(); ++i)
		{
			Task* task = sortedTasks[i];

			// Reuse the task index, set it to the tasks sorted index.
			task->m_index = static_cast<uint32_t>(i);

			m_jobs.emplace_back(task->CreateJob(m_priority));
		}

		for (size_t i = 0; i < sortedTasks.size(); ++i)
		{
			Task* task = sortedTasks[i];
			JobRef job = m_jobs[i];

			JobCounterRef waitCounter = JobSystem::CreateCounter();
			job->SetWaitCounter(waitCounter);

			// Add wait counter as an associated counter to dependencies.
			for (Task* dependency : task->GetDependencies())
			{
				// The task index is the sorted task index at this point.
				JobRef dependencyJob = m_jobs[dependency->m_index];

				dependencyJob->AddAssociatedCounter(waitCounter);
			}

			// Root job, add graph counter as associated counter
			if (task->GetRefCount() == 0)
			{
				job->AddAssociatedCounter(m_graphCounter);
			}
		}
	}

	void TaskGraph::ValidateDependencyChains()
	{
		// Detect cyclic dependencies with an iterative three-color (white/gray/black)
		// DFS. Gray means "currently an ancestor on the active path" - finding an edge
		// into a gray task is a back-edge, i.e. an actual cycle. Black tasks have already
		// been fully validated and are skipped, so every task is expanded at most once.
		enum class VisitState : uint8_t
		{
			White = 0,
			Gray,
			Black
		};

		struct StackFrame
		{
			Task* task;
			size_t dependencyIndex;
		};

		GlobalMemoryStackMark memMark;

		GlobalMemoryStackVector<VisitState> visitState;
		visitState.resize(m_tasks.size(), VisitState::White);

		GlobalMemoryStackVector<StackFrame> stack;
		stack.reserve(m_tasks.size());

		for (Task* startTask : m_tasks)
		{
			if (visitState[startTask->m_index] != VisitState::White)
			{
				continue;
			}

			visitState[startTask->m_index] = VisitState::Gray;
			stack.emplace_back(StackFrame{ startTask, 0 });

			while (!stack.empty())
			{
				StackFrame& top = stack.back();
				std::span<Task*> dependencies = top.task->GetDependencies();

				if (top.dependencyIndex >= dependencies.size())
				{
					visitState[top.task->m_index] = VisitState::Black;
					stack.pop_back();
					continue;
				}

				Task* dependency = dependencies[top.dependencyIndex];
				// Increment before pushing below - emplace_back can reallocate
				// the stack and invalidate the 'top' reference.
				++top.dependencyIndex;

				VisitState& depState = visitState[dependency->m_index];
				if (depState == VisitState::Gray)
				{
					VT_ASSERT_MSG(false, "Cyclic dependency detected in TaskGraph!");
					return;
				}

				if (depState == VisitState::White)
				{
					depState = VisitState::Gray;
					stack.emplace_back(StackFrame{ dependency, 0 });
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
