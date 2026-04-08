#pragma once

#include "FileSystemModule/Config.h"

#include <JobSystem/Job.h>
#include <JobSystem/JobSystem.h>

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
		VTFS_API IORequest(StringView name);
		virtual ~IORequest() = default;

		virtual void Execute() = 0;
		virtual IORequestResultCode GetResultCode() const = 0;

		VT_INLINE StringView GetName() const { return m_name; }

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
		VTFS_API void FreeRequest();

		std::atomic_int32_t m_refCount;
		StringView m_name;
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
		IORequestResult(IORequestResult&&) noexcept;
		IORequestResult& operator=(const IORequestResult&) noexcept = delete;
		IORequestResult& operator=(IORequestResult&&) noexcept;

		ResultType& GetResult();
		VT_INLINE IORequestResultCode GetResultCode() const 
		{
			JobSystem::WaitForCounter(m_assignedCounter);
			IORequestResultCode requestCode = m_ioRequest->GetResultCode();
			VT_ENSURE(requestCode != IORequestResultCode::Undefined);
			return requestCode; 
		}

	private:
		T* m_ioRequest = nullptr;
		JobCounterRef m_assignedCounter = nullptr;
	};

	template<typename T>
		requires(std::is_base_of_v<IORequest, T>)
	IORequestResult<T>::IORequestResult(JobCounterRef assignedCounter, T* ioRequest)
		: m_ioRequest(ioRequest),
		m_assignedCounter(assignedCounter)
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
	IORequestResult<T>::IORequestResult(IORequestResult&& other) noexcept
	{
		m_ioRequest = other.m_ioRequest;
		m_assignedCounter = other.m_assignedCounter;

		other.m_ioRequest = nullptr;
		other.m_assignedCounter = nullptr;
	}

	template<typename T>
		requires(std::is_base_of_v<IORequest, T>)
	IORequestResult<T>& IORequestResult<T>::operator=(IORequestResult&& other) noexcept
	{
		if (&other != this)
		{
			m_ioRequest = other.m_ioRequest;
			m_assignedCounter = other.m_assignedCounter;

			other.m_ioRequest = nullptr;
			other.m_assignedCounter = nullptr;
		}

		return *this;
	}

	template<typename T>
		requires(std::is_base_of_v<IORequest, T>)
	IORequestResult<T>::ResultType& IORequestResult<T>::GetResult()
	{
		JobSystem::WaitForCounter(m_assignedCounter);
		return m_ioRequest->GetResult();
	}
}
