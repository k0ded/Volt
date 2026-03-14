#include "jspch.h"

#include "JobSystem/IOThreads/IORequestAllocator.h"
#include "JobSystem/IOThreads/IORequest.h"

namespace Volt
{
	void IORequestAllocator::Free(IORequest* request)
	{
		request->~IORequest();

		IORequestMemory* memory = reinterpret_cast<IORequestMemory*>(request);
		m_allocator.Free(memory);
	}
}
