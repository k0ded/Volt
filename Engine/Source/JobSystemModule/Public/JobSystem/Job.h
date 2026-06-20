#pragma once

#include "JobSystem/Config.h"
#include "JobSystem/FiberCommon.h"
#include "JobSystem/FiberContext.h"

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Containers/ArrayView.h>
#include <CoreUtilities/Containers/VectorVariants.h>

#include <string_view>
#include <new>
#include <atomic>

namespace Volt
{
	class JobFiber;
	class Job;

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
			m_isCompleted.store(0, std::memory_order::seq_cst);
			m_waiterHead.store(nullptr, std::memory_order::seq_cst);
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
		friend class JobSystem;

		constexpr inline static Job* const WaitingListClosed = reinterpret_cast<Job*>(uintptr_t(1));

		// Used for the waiting list.
		std::atomic<class Job*> m_waiterHead = nullptr;
		std::atomic_int32_t m_counter = 0;
		std::atomic_int32_t m_referenceCount = 0;
		std::atomic_uint32_t m_isCompleted = 0;
	};

	using JobCounterRef = JobCounter*;

	struct JobStorage
	{
	public:
		JobStorage() = default;
		~JobStorage();

		template<typename Func>
		uint8_t* AllocateStorage();

		uint8_t* GetStorage();

	private:
		inline static constexpr size_t MaxSmallJobFuncSize = 64;

		uint8_t m_localStorage[MaxSmallJobFuncSize];
		uint8_t* m_heapStorage = nullptr;
	};

	class VTJS_API alignas(std::hardware_destructive_interference_size) Job
	{
	public:

		Job() = default;

		template<typename Func> void Create(StringView name, JobCounter* counter, JobCounter* waitCounter, ExecutionPriority priority, ExecutionPolicy executionPolicy, FiberStackSize stackSize, Func&& jobFunc);

		void Reset();

		VT_NODISCARD VT_INLINE StringView GetName() const { return m_jobName; }
		VT_NODISCARD VT_INLINE ArrayView<JobCounterRef> GetAssociatedCounters() const { return m_associatedCounters; }
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
		friend class TaskGraph;

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

		VT_NODISCARD VT_INLINE JobFuncBase* GetJobFunction() { return reinterpret_cast<JobFuncBase*>(m_jobStorage.GetStorage()); }

		void ExecuteInternal();
		void AddAssociatedCounter(JobCounterRef counter);
		void SetWaitCounter(JobCounterRef counter);

		bool m_allocated : 1 = false;

		ExecutionPolicy m_executionPolicy = ExecutionPolicy::WorkerThread;
		ExecutionPriority m_priority = ExecutionPriority::Critical;
		FiberStackSize m_stackSize;

		std::atomic_int32_t m_referenceCount = 0;

		InlineVector<JobCounter*, 1> m_associatedCounters;
		JobCounter* m_waitCounter = nullptr;
		JobFiber* m_assignedFiber = nullptr;

		// Used for the per job counter intrusive waiting list.
		Job* m_nextWaiter = nullptr;

		StringView m_jobName;
		JobStorage m_jobStorage;
	};

	using JobRef = Job*;

	template<typename Func>
	void Job::Create(StringView name, JobCounter* counter, JobCounter* waitCounter, ExecutionPriority priority, ExecutionPolicy executionPolicy, FiberStackSize stackSize, Func&& jobFunc)
	{
		m_jobName = name;

		if (counter)
		{
			m_associatedCounters = { counter };
		}

		m_waitCounter = waitCounter;
		m_executionPolicy = executionPolicy;
		m_priority = priority;
		m_stackSize = stackSize;

		void* storagePtr = m_jobStorage.AllocateStorage<Func>();
		new(storagePtr) JobFunc<std::remove_reference_t<Func>>(std::move(jobFunc));
		m_allocated = true;
	}

	template<typename Func>
	uint8_t* JobStorage::AllocateStorage()
	{
		constexpr size_t funcSize = sizeof(Func);

		if (funcSize <= MaxSmallJobFuncSize)
		{
			return &m_localStorage[0];
		}
		
		m_heapStorage = static_cast<uint8_t*>(Memory::Malloc(sizeof(Func)));
		return m_heapStorage;
	}
}
