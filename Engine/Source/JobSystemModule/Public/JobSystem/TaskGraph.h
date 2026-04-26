#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/JobSystem.h"
#include "JobSystem/FiberCommon.h"

#include <CoreUtilities/Allocators/PagedAtomicLinearAllocator.h>

#include <unordered_set>

namespace Volt
{
	class VTJS_API TaskGraphAllocator
	{
	public:
		TaskGraphAllocator() = default;
		~TaskGraphAllocator();

		VT_DELETE_COPY_MOVE(TaskGraphAllocator);

		template<typename T, typename... Args>
		T* CreateTask(Args&&... args)
		{
			void* dataPtr = AllocateBytes(sizeof(T));
			T* valuePtr = new (dataPtr) T(std::forward<Args>(args)...);

			TaskDestructor& destructor = m_taskDestructors.emplace_back();
			destructor.dataPtr = dataPtr;
			destructor.destructor = [](void* ptr) 
			{
				std::launder(reinterpret_cast<T*>(ptr))->~T();
			};

			return valuePtr;
		}

	private:
		struct TaskDestructor
		{
			std::function<void(void*)> destructor;
			void* dataPtr = nullptr;
		};

		void* AllocateBytes(size_t size);

		PagedAtomicLinearAllocator<1024> m_allocator;
		Vector<TaskDestructor> m_taskDestructors;
	};

	class VTJS_API TaskGraph
	{
	public:
		class VTJS_API Task
		{
		public:
			virtual ~Task() = default;

			void AddDependency(Task* dependency);
			void AddDependencies(std::span<Task*> dependencies);
			void AddDependencies(std::initializer_list<Task*> dependencies);

			VT_INLINE uint32_t GetRefCount() const
			{
				return m_referenceCount;
			}

			VT_INLINE void IncRef()
			{
				m_referenceCount++;
			}

			VT_INLINE std::span<Task*> GetDependencies()
			{
				return m_dependencies;
			}

		protected:
			friend class TaskGraph;
			virtual JobRef CreateJob(ExecutionPriority priority, JobCounterRef associatedCounter, JobCounterRef waitCounter) = 0;
			virtual JobRef CreateJob(ExecutionPriority priority) = 0;

			StringView m_name;
			uint32_t m_referenceCount = 0;

			// #TODO_Ivar: Switch to a sparse set when we have one.
			Vector<Task*> m_dependencies;
		
		};

		TaskGraph(ExecutionPriority priority, size_t numExpectedTasks = 128);
		~TaskGraph();

		VT_DELETE_COPY_MOVE(TaskGraph);

		template<typename Func> TaskGraph::Task* AddTask(StringView name, Func&& func, FiberStackSize stackSize = FiberStackSize::KB16);
		template<typename Func> TaskGraph::Task* AddTaskWithDependencies(StringView name, std::span<TaskGraph::Task*> dependencies, Func&& func, FiberStackSize stackSize = FiberStackSize::KB16);
		template<typename Func> TaskGraph::Task* AddTaskWithDependencies(StringView name, std::initializer_list<TaskGraph::Task*> dependencies, Func&& func, FiberStackSize stackSize = FiberStackSize::KB16);

		void Execute();
		JobCounterRef ExecuteAndExtractCounter();
		void ExecuteAndWait();
		void Wait();

	private:
		void Compile();

		template<typename Func>
		class TaskImpl : public Task
		{
		public:
			TaskImpl(Func&& inFunc, FiberStackSize inStackSize)
				: func(std::move(inFunc)),
				stackSize(inStackSize)
			{ }

			~TaskImpl() override = default;
			
			JobRef CreateJob(ExecutionPriority priority, JobCounterRef associatedCounter, JobCounterRef waitCounter) override
			{
				return JobSystem::CreateJob(m_name, priority, ExecutionPolicy::WorkerThread, associatedCounter, waitCounter, std::move(func), stackSize);
			}

			JobRef CreateJob(ExecutionPriority priority) override
			{
				return JobSystem::CreateJobNoCounters(m_name, priority, ExecutionPolicy::WorkerThread, std::move(func), stackSize);
			}

			Func func;
			FiberStackSize stackSize;
		};

		bool m_isExecuted = false;
		ExecutionPriority m_priority;
		size_t m_numExpectedTasks;
		JobCounterRef m_graphCounter = nullptr;
		TaskGraphAllocator m_allocator;

		Vector<Task*> m_tasks; 
		Vector<JobRef> m_jobs;
	};

	template<typename Func>
	TaskGraph::Task* TaskGraph::AddTask(StringView name, Func&& func, FiberStackSize stackSize)
	{
		TaskImpl<Func>* taskDescription = m_allocator.CreateTask<TaskImpl<Func>>(std::move(func), stackSize);
		taskDescription->m_name = name;

		m_tasks.emplace_back(taskDescription);
		return taskDescription;
	}

	template<typename Func>
	TaskGraph::Task* TaskGraph::AddTaskWithDependencies(StringView name, std::span<TaskGraph::Task*> dependencies, Func&& func, FiberStackSize stackSize)
	{
		TaskImpl<Func>* taskDescription = m_allocator.CreateTask<TaskImpl<Func>>(std::move(func), stackSize);
		taskDescription->m_name = name;
		taskDescription->AddDependencies(dependencies);

		m_tasks.emplace_back(taskDescription);
		return taskDescription;
	}

	template<typename Func>
	TaskGraph::Task* TaskGraph::AddTaskWithDependencies(StringView name, std::initializer_list<TaskGraph::Task*> dependencies, Func&& func, FiberStackSize stackSize)
	{
		TaskImpl<Func>* taskDescription = m_allocator.CreateTask<TaskImpl<Func>>(std::move(func), stackSize);
		taskDescription->m_name = name;
		taskDescription->AddDependencies(dependencies);

		m_tasks.emplace_back(taskDescription);
		return taskDescription;
	}
}
