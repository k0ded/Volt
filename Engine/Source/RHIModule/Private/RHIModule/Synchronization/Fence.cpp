#include "rhipch.h"

#include "RHIModule/Synchronization/Fence.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<Fence> Fence::Create(const FenceCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateFence(createInfo);
	}
}
