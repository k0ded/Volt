#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/FiberCommon.h"
#include "JobSystem/FiberContext.h"

#include <CoreUtilities/Core.h>

#include <string_view>

namespace Volt
{
	class JobFiber;

	enum class ExecutionPolicy : uint8_t
	{
		MainThread = 0,
		WorkerThread
	};

	enum class ExecutionPriority : uint8_t
	{
		Latent = 0,
		Render,
		Critical,
		Immediate,
		Num
	};

	// Make sure it is aligned to avoid false sharing.
	class alignas(std::hardware_destructive_interference_size) VTJS_API JobCounter
	{
	public:
		VT_INLINE int32_t Increment(int32_t increment = 1)
		{
			return m_counter.fetch_add(increment);
		}

		VT_INLINE int32_t Decrement(int32_t decrement = 1)
		{
			int32_t prevCount = m_counter.fetch_sub(decrement);

			// Only call once when the counter is finished.
			if (prevCount == 1)
			{
				NotifyCounterReady();

				m_isCompleted.store(1, std::memory_order::relaxed);
				m_isCompleted.notify_all();
			}
			return prevCount;
		}

		VT_INLINE int32_t GetCounter() const
		{
			return m_counter.load(std::memory_order::relaxed);
		}

		VT_INLINE bool IsActive() const
		{
			return m_counter.load(std::memory_order::relaxed) >= 0;
		}

		VT_INLINE bool IsCompleted() const
		{
			return m_counter.load(std::memory_order::relaxed) <= 0;
		}

		VT_INLINE void Reset()
		{
			m_counter.store(0, std::memory_order::seq_cst);
		}

		VT_INLINE uint32_t GetRefCount() const
		{
			return m_referenceCount.load(std::memory_order::relaxed);
		}

		VT_INLINE void IncRef()
		{
			m_referenceCount.fetch_add(1, std::memory_order::relaxed);
		}

		VT_INLINE void WaitForCounterLocking()
		{
			m_isCompleted.wait(0, std::memory_order::relaxed);
		}

		// Needs to be in a cpp file as it interacts with 
		// the job system.
		void DecRef();
		void NotifyCounterReady();

	private:
		std::atomic_int32_t m_counter = 0;
		std::atomic_int32_t m_referenceCount = 0;
		std::atomic_uint32_t m_isCompleted = 0;
	};

	using JobCounterRef = JobCounter*;

	class VTJS_API alignas(std::hardware_destructive_interference_size) Job
	{
	public:
		inline static constexpr size_t MaxJobFuncSize = 1024;

		Job() = default;

		template<typename Func> void Create(std::string_view name, JobCounter* counter, JobCounter* waitCounter, ExecutionPriority priority, ExecutionPolicy executionPolicy, FiberStackSize stackSize, Func&& jobFunc);

		void Reset();

		VT_NODISCARD VT_INLINE std::string_view GetName() const { return m_jobName; }
		VT_NODISCARD VT_INLINE JobCounter* GetCounter() const { return m_counter; }
		VT_NODISCARD VT_INLINE JobCounter* GetWaitCounter() const { return m_waitCounter; }
		VT_NODISCARD VT_INLINE JobFiber* GetAssignedFiber() const { return m_assignedFiber; }
		VT_NODISCARD VT_INLINE ExecutionPolicy GetExecutionPolicy() const { return m_executionPolicy; }
		VT_NODISCARD VT_INLINE ExecutionPriority GetPriority() const { return m_priority; }
		VT_NODISCARD VT_INLINE FiberStackSize GetStackSize() const { return m_stackSize; }

		VT_INLINE void IncRef()
		{
			m_referenceCount.fetch_add(1, std::memory_order::relaxed);
		}

		// Needs to be in a cpp file as it interacts with 
		// the job system.
		void DecRef();

	private:
		friend void ExecuteFiber(void* userData);
		friend class JobSystem;
		friend class JobFiber;

		struct JobFuncBase
		{
			virtual ~JobFuncBase() = default;
			virtual void Execute() = 0;
		};

		template<typename Func>
		struct JobFunc : public JobFuncBase
		{
			JobFunc(Func&& inFunc)
				: func(std::move(inFunc))
			{}

			~JobFunc() override = default;
			void Execute() override { func(); }

			Func func;
		};

		VT_NODISCARD VT_INLINE JobFuncBase* GetJobFunction() { return reinterpret_cast<JobFuncBase*>(&m_funcStorage); }

		void ExecuteInternal();

		bool m_allocated : 1 = false;

		ExecutionPolicy m_executionPolicy = ExecutionPolicy::WorkerThread;
		ExecutionPriority m_priority = ExecutionPriority::Critical;
		std::atomic_int32_t m_referenceCount = 0;

		JobCounter* m_counter = nullptr;
		JobCounter* m_waitCounter = nullptr;
		JobFiber* m_assignedFiber = nullptr;
		FiberStackSize m_stackSize;

		std::string_view m_jobName;

		// #TODO_Ivar: Figure out if we should reduce this to get a better total size.
		uint8_t m_funcStorage[MaxJobFuncSize];
	};

	using JobRef = Job*;

	template<typename Func>
	void Job::Create(std::string_view name, JobCounter* counter, JobCounter* waitCounter, ExecutionPriority priority, ExecutionPolicy executionPolicy, FiberStackSize stackSize, Func&& jobFunc)
	{
		static_assert(sizeof(Func) <= Job::MaxJobFuncSize);

		m_jobName = name;
		m_counter = counter;
		m_waitCounter = waitCounter;
		m_executionPolicy = executionPolicy;
		m_priority = priority;
		m_stackSize = stackSize;

		void* storagePtr = &m_funcStorage;
		new(storagePtr) JobFunc<std::remove_reference_t<Func>>(std::move(jobFunc));
		m_allocated = true;
	}
}
