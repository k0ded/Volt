#include "rhipch.h"

#include "RHIModule/Synchronization/Semaphore.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<Semaphore> Semaphore::Create(const SemaphoreCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateSemaphore(createInfo);
	}
}
