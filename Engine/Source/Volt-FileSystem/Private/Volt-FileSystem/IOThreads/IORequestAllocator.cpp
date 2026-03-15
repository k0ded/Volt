#include "Volt-FileSystem/IOThreads/IORequestAllocator.h"
#include "Volt-FileSystem/IOThreads/IORequest.h"

namespace Volt
{
	void IORequestAllocator::Free(IORequest* request)
	{
		request->~IORequest();

		IORequestMemory* memory = reinterpret_cast<IORequestMemory*>(request);
		m_allocator.Free(memory);
	}
}
