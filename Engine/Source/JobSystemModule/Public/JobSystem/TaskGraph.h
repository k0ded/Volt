#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/JobSystem.h"

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
			new (dataPtr) T(std::forward<Args>(args)...);

			TaskDestructor& destructor = m_taskDestructors.emplace_back();
			destructor.dataPtr = dataPtr;
			destructor.destructor = [](void* ptr) 
			{
				reinterpret_cast<T*>(ptr)->~T();
			};

			return reinterpret_cast<T*>(dataPtr);
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
			virtual JobRef CreateJob(ExecutionPriority priority, JobCounterRef counter) = 0;
			virtual JobRef CreateJobAsDependency(ExecutionPriority priority, JobRef dependant) = 0;

			std::string_view m_name;
			uint32_t m_referenceCount = 0;

			// #TODO_Ivar: Switch to a sparse set when we have one.
			Vector<Task*> m_dependencies;
		
		};

		TaskGraph(ExecutionPriority priority, size_t numExpectedTasks = 128);
		~TaskGraph();

		VT_DELETE_COPY_MOVE(TaskGraph);

		template<typename Func>
		TaskGraph::Task* AddTask(std::string_view name, Func&& func);

		template<typename Func>
		TaskGraph::Task* AddTaskWithDependencies(std::string_view name, std::span<TaskGraph::Task*> dependencies, Func&& func);

		template<typename Func>
		TaskGraph::Task* AddTaskWithDependencies(std::string_view name, std::initializer_list<TaskGraph::Task*> dependencies, Func&& func);

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
			TaskImpl(Func&& inFunc)
				: func(std::move(inFunc))
			{ }

			~TaskImpl() override = default;
			
			JobRef CreateJob(ExecutionPriority priority, JobCounterRef counter) override
			{
				return JobSystem::CreateJob(m_name, priority, counter, std::move(func));
			}

			JobRef CreateJobAsDependency(ExecutionPriority priority, JobRef dependant) override
			{
				return JobSystem::CreateJobAsDependency(m_name, dependant, std::move(func));
			}

			Func func;
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
	TaskGraph::Task* TaskGraph::AddTask(std::string_view name, Func&& func)
	{
		TaskImpl<Func>* taskDescription = m_allocator.CreateTask<TaskImpl<Func>>(std::move(func));
		taskDescription->m_name = name;

		m_tasks.emplace_back(taskDescription);
		return taskDescription;
	}

	template<typename Func>
	TaskGraph::Task* TaskGraph::AddTaskWithDependencies(std::string_view name, std::span<TaskGraph::Task*> dependencies, Func&& func)
	{
		TaskImpl<Func>* taskDescription = m_allocator.CreateTask<TaskImpl<Func>>(std::move(func));
		taskDescription->m_name = name;
		taskDescription->AddDependencies(dependencies);

		m_tasks.emplace_back(taskDescription);
		return taskDescription;
	}

	template<typename Func>
	TaskGraph::Task* TaskGraph::AddTaskWithDependencies(std::string_view name, std::initializer_list<TaskGraph::Task*> dependencies, Func&& func)
	{
		TaskImpl<Func>* taskDescription = m_allocator.CreateTask<TaskImpl<Func>>(std::move(func));
		taskDescription->m_name = name;
		taskDescription->AddDependencies(dependencies);

		m_tasks.emplace_back(taskDescription);
		return taskDescription;
	}
}
