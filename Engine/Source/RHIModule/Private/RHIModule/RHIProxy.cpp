#include "rhipch.h"
#include "RHIProxy.h"

namespace Volt::RHI
{
	RHIProxy::~RHIProxy()
	{
		s_instance->m_frameCapture = nullptr;
	}

	void RHIProxy::SetFrameCapture(Ref<FrameCapture> frameCapture)
	{
		m_frameCapture = frameCapture;
	}

	Weak<FrameCapture> RHIProxy::GetFrameCapture()
	{
		return s_instance->m_frameCapture;
	}

	RHIProxy::RHIProxy()
	{
		s_instance = this;
	}
}
