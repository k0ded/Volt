#include "jspch.h"

#include "JobSystem/IOThreads/IORequest.h"
#include "JobSystem/IOThreads/IOThreads.h"

namespace Volt
{
	IORequest::IORequest(std::string_view name)
		: m_name(name)
	{
	}

	void IORequest::FreeRequest()
	{
		IOThreads::s_instance->FreeIORequest(this);
	}
}
