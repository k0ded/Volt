#include "rhipch.h"

#include "RHIModule/Graphics/GraphicsDevice.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<GraphicsDevice> GraphicsDevice::Create(const GraphicsDeviceCreateInfo& deviceInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer)
	{
		return RHIModule::GetInstance().CreateGraphicsDevice(deviceInfo, physicalGraphicsDevice, enableDebugLayer);
	}

	GraphicsDevice::GraphicsDevice()
	{
	}
}
