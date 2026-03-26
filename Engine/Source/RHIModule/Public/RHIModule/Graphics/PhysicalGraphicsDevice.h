#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

namespace Volt::RHI
{
	class VTRHI_API PhysicalGraphicsDevice : public RHIInterface
	{
	public:
		VT_DELETE_COPY_MOVE(PhysicalGraphicsDevice);
		~PhysicalGraphicsDevice() override = default;

		[[nodiscard]] virtual const DeviceVendor GetDeviceVendor() const = 0;
		[[nodiscard]] virtual StringView GetDeviceName() const = 0;

		static IntRef<PhysicalGraphicsDevice> Create(const PhysicalDeviceCreateInfo& deviceInfo, bool enableDebugLayer);

	protected:
		PhysicalGraphicsDevice() = default;
	};
}
