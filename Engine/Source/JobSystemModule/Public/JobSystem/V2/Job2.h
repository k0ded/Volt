#pragma once

#include <CoreUtilities/Core.h>

namespace Volt
{
	class JobCounter
	{
	public:
		VT_INLINE int32_t Increment(int32_t increment = 1)
		{
			return m_counter.fetch_add(increment, std::memory_order::relaxed);
		}

		VT_INLINE int32_t Decrement(int32_t decrement = 1)
		{
			return m_counter.fetch_sub(decrement, std::memory_order::relaxed);
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

	private:
		std::atomic<int32_t> m_counter = 0;
	};

	class alignas(64) Job2
	{
	public:
		Job2() = default;

		template<typename Func>
		void Create(std::string_view name, JobCounter* counter, Func&& jobFunc);

		void Execute();

		VT_NODISCARD VT_INLINE std::string_view GetName() const { return m_jobName; }
		VT_NODISCARD VT_INLINE JobCounter* GetCounter() const { return m_counter; }

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

		bool m_allocated = 0;
		JobCounter* m_counter = nullptr;
		std::string_view m_jobName;
		uint8_t m_funcStorage[1024];
	};

	template<typename Func>
	void Job2::Create(std::string_view name, JobCounter* counter, Func&& jobFunc)
	{
		static_assert(sizeof(Func) < 1024);

		m_jobName = name;
		m_counter = counter;

		void* storagePtr = &m_funcStorage;
		new(storagePtr) JobFunc<std::remove_reference_t<Func>>(std::move(jobFunc));
		m_allocated = true;
	}

	void Job2::Execute()
	{
		if (m_allocated)
		{
			JobFuncBase* funcPtr = reinterpret_cast<JobFuncBase*>(&m_funcStorage);
			funcPtr->Execute();

			// Destroy the function.
			funcPtr->~JobFuncBase();
		}
	}
}
