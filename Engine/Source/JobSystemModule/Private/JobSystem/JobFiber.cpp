#include "jspch.h"

#include "JobSystem/JobFiber.h"
#include "JobSystem/JobSystem.h"
#include "JobSystem/Asm/FiberContext.h"

namespace Volt
{
	void EmptyCallback(void*)
	{}

	void ExecuteFiber(void* userdata)
	{
		Job* jobPtr = reinterpret_cast<Job*>(userdata);
		jobPtr->ExecuteInternal();

		// Job finished execution
		JobFiber* assignedFiber = jobPtr->m_assignedFiber;
		JobSystem::s_instance->FinishJob(assignedFiber, jobPtr);
	}

	JobFiber::JobFiber(const String& fiberName, int32_t id)
		: m_name(fiberName),
		m_id(id)
	{
	}

	bool JobFiber::ExecuteJob(Job* job)
	{
		VT_ASSERT(m_currentJob == nullptr);
		VT_ASSERT(!m_stack.IsValid());

		bool result = JobSystem::s_instance->AllocateStack(job->GetStackSize(), m_stack);
		if (!result)
		{
			return false;
		}

		m_currentJob = job;
		m_currentJob->m_assignedFiber = this;

		memset(&m_executionContext, 0, sizeof(FiberContext));
		m_executionContext.rip = reinterpret_cast<void*>(&ExecuteFiber);
		m_executionContext.rsp = m_stack.GetStackPointer();
		m_executionContext.stackBase = m_stack.GetStackBase();
		m_executionContext.stackLimit = m_stack.GetStackLimit();
		m_executionContext.deallocationStack = m_stack.GetDeallocationStack();

		// Inherit the FPU state of the current thread.
		FiberThreadFPState fpState;
		FiberThreadGetFPState(&fpState);

		m_executionContext.mxcsr = fpState.mxcsr;
		m_executionContext.fpucw = fpState.fpucw;

		m_executionContext.userdata = job;

		VT_PROFILE_FIBER_ENTER(m_name.c_str());
		FiberSetContext(&m_executionContext);

		return true;
	}

	void JobFiber::ContinueExecution()
	{
		VT_ASSERT(m_currentJob);

		// Move thread back to the fiber
		m_executionContext.userdata = m_currentJob;
		VT_PROFILE_FIBER_ENTER(m_name.c_str());
		FiberSetContext(&m_executionContext);
	}

	void JobFiber::Free()
	{
		JobSystem::s_instance->FreeStack(m_stack);
		m_stack = {};

		m_currentJob = nullptr;
	}
}
