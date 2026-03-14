#pragma once

#include "JobSystem/Job.h"

#include <concepts>

namespace Volt
{
	enum class IORequestResultCode : uint8_t
	{
		Undefined = 0,
		Success,
		Failure
	};

	class IORequest
	{
	public:
		IORequest(std::string_view name);
		virtual ~IORequest() = default;

		virtual void Execute() = 0;
		virtual IORequestResultCode GetResultCode() const = 0;

		VT_INLINE std::string_view GetName() const { return m_name; }

		VT_INLINE void IncRef() 
		{ 
			m_refCount.fetch_add(1, std::memory_order::relaxed); 
		}

		VT_INLINE void DecRef()
		{
			int32_t prevCount = m_refCount.fetch_sub(1, std::memory_order::release);
			if (prevCount == 1)
			{
				std::atomic_thread_fence(std::memory_order::acquire);
				FreeRequest();
			}
		}

	private:
		VTJS_API void FreeRequest();

		std::atomic_int32_t m_refCount;
		std::string_view m_name;
	};

	template<typename T>
		requires(std::is_base_of_v<IORequest, T>)
	class IORequestResult
	{
	public:
		using ResultType = typename T::ResultType;

		IORequestResult(JobCounterRef assignedCounter, T* ioRequest);
		~IORequestResult();

		IORequestResult(const IORequestResult&) noexcept = delete;
		IORequestResult(IORequestResult&&) noexcept = delete;
		IORequestResult& operator=(const IORequestResult&) noexcept = delete;
		IORequestResult& operator=(IORequestResult&&) noexcept = delete;

		ResultType& GetResult();
		VT_INLINE IORequestResultCode GetResultCode() const { return m_ioRequest->GetResultCode(); }

	private:
		T* m_ioRequest;
		JobCounterRef m_assignedCounter;
	};

	template<typename T>
		requires(std::is_base_of_v<IORequest, T>)
	IORequestResult<T>::IORequestResult(JobCounterRef assignedCounter, T* ioRequest)
		: m_assignedCounter(assignedCounter),
		m_ioRequest(ioRequest)
	{
		m_ioRequest->IncRef();
		m_assignedCounter->IncRef();
	}

	template<typename T>
		requires(std::is_base_of_v<IORequest, T>)
	IORequestResult<T>::~IORequestResult()
	{
		JobSystem::DestroyCounter(m_assignedCounter);
		m_ioRequest->DecRef();
	}

	template<typename T>
		requires(std::is_base_of_v<IORequest, T>)
	IORequestResult<T>::ResultType& IORequestResult<T>::GetResult()
	{
		JobSystem::WaitForCounter(m_assignedCounter);
		return m_ioRequest->GetResult();
	}
}
