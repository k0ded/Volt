#include "FileSystemModule/IOThreads/IORequest.h"
#include "FileSystemModule/IOThreads/IOThreads.h"

namespace Volt
{
	IORequest::IORequest(StringView name)
		: m_name(name),
		m_refCount(0)
	{
	}

	void IORequest::FreeRequest()
	{
		IOThreads::s_instance->FreeIORequest(this);
	}
}
