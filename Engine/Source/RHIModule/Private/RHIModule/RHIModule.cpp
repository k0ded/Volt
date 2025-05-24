#include "rhipch.h"
#include "RHIModule.h"

namespace Volt::RHI
{
	RHIModule::~RHIModule()
	{
		s_instance->m_frameCapture = nullptr;
	}

	void RHIModule::SetFrameCapture(Ref<FrameCapture> frameCapture)
	{
		m_frameCapture = frameCapture;
	}

	Weak<FrameCapture> RHIModule::GetFrameCapture()
	{
		return s_instance->m_frameCapture;
	}

	RHIModule::RHIModule()
	{
		s_instance = this;
	}
}
