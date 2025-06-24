#include "rhipch.h"

#include "RHIModule/Graphics/DeviceQueue.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<DeviceQueue> DeviceQueue::Create(const DeviceQueueCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateDeviceQueue(createInfo);
	}
}
