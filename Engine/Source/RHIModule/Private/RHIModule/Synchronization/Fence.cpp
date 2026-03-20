#include "rhipch.h"

#include "RHIModule/Synchronization/Fence.h"

#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<Fence> Fence::Create()
	{
		return RHIModule::GetInstance().CreateFence();
	}
}
