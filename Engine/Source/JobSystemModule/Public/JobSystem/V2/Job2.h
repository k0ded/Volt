#pragma once

#include <CoreUtilities/Core.h>

namespace Volt
{
	class alignas(64) Job2
	{
	public:
		Job2() = default;

		template<typename Func>
		void Create(std::string_view name, Func&& jobFunc);

		void Execute();

		VT_NODISCARD VT_INLINE std::string_view GetName() const { return m_jobName; }

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
		std::string_view m_jobName;
		uint8_t m_funcStorage[1024];
	};

	template<typename Func>
	void Job2::Create(std::string_view name, Func&& jobFunc)
	{
		static_assert(sizeof(Func) < 1024);

		m_jobName = name;

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
