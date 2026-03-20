#include "rhipch.h"

#include "RHIModule/Graphics/PhysicalGraphicsDevice.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<PhysicalGraphicsDevice> PhysicalGraphicsDevice::Create(const PhysicalDeviceCreateInfo& deviceInfo, bool enableDebugLayer)
	{
		return RHIModule::GetInstance().CreatePhysicalGraphicsDevice(deviceInfo, enableDebugLayer);
	}
}
