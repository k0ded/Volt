#include "rhipch.h"

#include "RHIModule/Images/SamplerState.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<SamplerState> SamplerState::Create(const SamplerStateCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateSamplerState(createInfo);
	}
}
