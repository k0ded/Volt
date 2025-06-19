#include "rhipch.h"

#include "RHIModule/Graphics/PhysicalGraphicsDevice.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<PhysicalGraphicsDevice> PhysicalGraphicsDevice::Create(const PhysicalDeviceCreateInfo& deviceInfo)
	{
		return RHIModule::GetInstance().CreatePhysicalGraphicsDevice(deviceInfo);
	}
}
