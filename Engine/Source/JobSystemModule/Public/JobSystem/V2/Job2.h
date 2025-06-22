#pragma once

#include "JobSystem/Config.h"

#include <CoreUtilities/Core.h>

namespace Volt
{
	class VTJS_API JobCounter
	{
	public:
		VT_INLINE int32_t Increment(int32_t increment = 1)
		{
			return m_counter.fetch_add(increment);
		}

		VT_INLINE int32_t Decrement(int32_t decrement = 1)
		{
			return m_counter.fetch_sub(decrement);
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

		// Needs to be in a cpp file as it interacts with 
		// the job system.
		void DecRef();

	private:
		std::atomic<int32_t> m_counter = 0;
		std::atomic<uint32_t> m_referenceCount = 0;
	};

	class VTJS_API alignas(64) Job2
	{
	public:
		Job2() = default;

		template<typename Func> void Create(std::string_view name, JobCounter* counter, JobCounter* waitCounter, Func&& jobFunc);

		void Execute();
		void Reset();

		VT_NODISCARD VT_INLINE std::string_view GetName() const { return m_jobName; }
		VT_NODISCARD VT_INLINE JobCounter* GetCounter() const { return m_counter; }
		VT_NODISCARD VT_INLINE JobCounter* GetWaitCounter() const { return m_waitCounter; }

		VT_INLINE void IncRef()
		{
			m_referenceCount.fetch_add(1, std::memory_order::relaxed);
		}

		// Needs to be in a cpp file as it interacts with 
		// the job system.
		void DecRef();

	private:
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
			{ }

			~JobFunc() override = default;
			void Execute() override { func(); }
		
			Func func;
		};

		bool m_allocated = false;
		std::atomic<uint32_t> m_referenceCount = 0;
		JobCounter* m_counter = nullptr;
		JobCounter* m_waitCounter = nullptr;
		std::string_view m_jobName;
		uint8_t m_funcStorage[1024];
	};

	template<typename Func>
	void Job2::Create(std::string_view name, JobCounter* counter, JobCounter* waitCounter, Func&& jobFunc)
	{
		static_assert(sizeof(Func) < 1024);

		m_jobName = name;
		m_counter = counter;
		m_waitCounter = waitCounter;

		void* storagePtr = &m_funcStorage;
		new(storagePtr) JobFunc<std::remove_reference_t<Func>>(std::move(jobFunc));
		m_allocated = true;
	}
}
