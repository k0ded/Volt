#include "rhipch.h"

#include "RHIModule/Synchronization/Semaphore.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<Semaphore> Semaphore::Create()
	{
		return RHIModule::GetInstance().CreateSemaphore();
	}
}
