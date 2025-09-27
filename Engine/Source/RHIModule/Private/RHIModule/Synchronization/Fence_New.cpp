#include "rhipch.h"

#include "RHIModule/Synchronization/Fence_New.h"

#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<Fence_New> Fence_New::Create()
	{
		return RHIModule::GetInstance().CreateFence();
	}
}
