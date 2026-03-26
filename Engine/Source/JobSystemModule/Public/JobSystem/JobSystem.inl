#pragma once

#include "JobSystem/JobSystem.h"

namespace Volt
{
	template<typename Func>
	VT_NODISCARD Job* JobSystem::CreateJob(StringView jobName, ExecutionPriority priority, Func&& func, FiberStackSize stackSize)
	{
		return CreateJob(jobName, priority, ExecutionPolicy::WorkerThread, std::move(func), stackSize);
	}

	template<typename Func>
	VT_NODISCARD Job* JobSystem::CreateJob(StringView jobName, ExecutionPriority priority, JobCounter* associatedCounter, Func&& func, FiberStackSize stackSize)
	{
		return CreateJob(jobName, priority, ExecutionPolicy::WorkerThread, associatedCounter, std::move(func), stackSize);
	}

	template<typename Func>
	VT_NODISCARD Job* JobSystem::CreateJob(StringView jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, Func&& func, FiberStackSize stackSize)
	{
		return CreateJob(jobName, priority, executionPolicy, nullptr, std::move(func), stackSize);
	}

	template<typename Func>
	VT_NODISCARD Job* JobSystem::CreateJob(StringView jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, JobCounter* associatedCounter, Func&& func, FiberStackSize stackSize)
	{
		// Will create a wait counter for us
		return CreateJob(jobName, priority, executionPolicy, associatedCounter, nullptr, std::move(func), stackSize);
	}

	template<typename Func>
	VT_NODISCARD Job* JobSystem::CreateJob(StringView jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, JobCounter* associatedCounter, JobCounter* waitCounter, Func&& func, FiberStackSize stackSize)
	{
		VT_PROFILE_FUNCTION();

		if (!associatedCounter)
		{
			associatedCounter = s_instance->AllocateCounter(false);
		}
		VT_ASSERT(associatedCounter->IsActive());

		if (!waitCounter)
		{
			waitCounter = s_instance->AllocateCounter();
		}
		else
		{
			waitCounter->IncRef();
		}
		VT_ASSERT(waitCounter != associatedCounter);

		Job* newJob = s_instance->AllocateJob();
		associatedCounter->Increment();
		associatedCounter->IncRef();

		newJob->Create(jobName, associatedCounter, waitCounter, priority, executionPolicy, stackSize, std::move(func));
		return newJob;
	}

	template<typename Func>
	VT_NODISCARD Job* JobSystem::CreateJobAsDependency(StringView jobName, Job* dependantJob, Func&& func, FiberStackSize stackSize)
	{
		// Inherit the priority.
		return CreateJob(jobName, dependantJob->GetPriority(), dependantJob->GetWaitCounter(), std::move(func), stackSize);
	}

	template<typename Func>
	VT_NODISCARD Job* JobSystem::CreateJobWithDependency(StringView jobName, ExecutionPriority priority, ExecutionPolicy executionPolicy, Job* dependencyJob, Func&& func, FiberStackSize stackSize)
	{
		return CreateJob(jobName, priority, executionPolicy, nullptr, dependencyJob->GetCounter(), std::move(func), stackSize);
	}
}
