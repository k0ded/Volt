#pragma once

#include "JobSystem/Job.h"
#include "JobSystem/JobSystem.h"

#include <CoreUtilities/Core.h>

namespace Volt
{
	template<typename Type>
	class JobFuture
	{
	public:
		JobFuture() = default;

		JobFuture(const JobFuture& other)
			: m_associatedCounter(other.m_associatedCounter),
			m_value(other.m_value)
		{
			if (m_associatedCounter)
			{
				m_associatedCounter->IncRef();
			}
		}

		JobFuture(JobFuture&& other)
			: m_associatedCounter(std::move(other.m_associatedCounter)),
			m_value(std::move(other.m_value))
		{ 
			other.m_associatedCounter = nullptr;
		}

		~JobFuture()
		{
			// Remove the ref that was added when the promise created it.
			if (m_associatedCounter)
			{
				JobSystem::DestroyCounter(m_associatedCounter);
			}
		}

		VT_INLINE JobFuture& operator=(const JobFuture& other)
		{
			m_associatedCounter = other.m_associatedCounter;
			m_value = other.m_value;

			if (m_associatedCounter)
			{
				m_associatedCounter->IncRef();
			}

			return *this;
		}

		VT_INLINE JobFuture& operator=(JobFuture&& other)
		{
			m_associatedCounter = std::move(other.m_associatedCounter);
			m_value = std::move(other.m_value);

			other.m_associatedCounter = nullptr;

			return *this;
		}

		VT_INLINE const Type& Get() const
		{
			VT_ENSURE(m_associatedCounter);
			JobSystem::WaitForCounter(m_associatedCounter);
			return *m_value;
		}

		VT_INLINE void WaitForCompletion()
		{
			VT_ENSURE(m_associatedCounter);
			JobSystem::WaitForCounter(m_associatedCounter);
		}

	private:
		template<typename T>
		friend class JobPromise;

		JobCounterRef m_associatedCounter = nullptr;
		Ref<Type> m_value;
	};

	template<typename Type>
	class JobPromise
	{
	public:
		JobPromise()
		{
			m_value = CreateRef<Type>();
		}

		~JobPromise()
		{
			if (m_associatedCounter)
			{
				JobSystem::DestroyCounter(m_associatedCounter);
			}
		}

		VT_INLINE const Type& Get() const
		{
			VT_ENSURE(m_associatedCounter);
			JobSystem::WaitForCounter(m_associatedCounter);
			return *m_value;
		}

		VT_INLINE void WaitForCompletion()
		{
			VT_ENSURE(m_associatedCounter);
			JobSystem::WaitForCounter(m_associatedCounter);
		}

		VT_INLINE void SetAssociatedCounter(JobCounterRef counter)
		{
			m_associatedCounter = counter;
		}

		VT_INLINE void SetValue(const Type& value)
		{
			*m_value = value;
		}

		VT_INLINE JobFuture<Type> GetFuture()
		{
			VT_ENSURE(m_associatedCounter);

			JobFuture<Type> future;
			future.m_value = m_value;
			future.m_associatedCounter = m_associatedCounter;

			// Add a ref here for the future.
			m_associatedCounter->IncRef();

			return future;
		}

	private:
		JobCounterRef m_associatedCounter = nullptr;
		Ref<Type> m_value;
	};

}
