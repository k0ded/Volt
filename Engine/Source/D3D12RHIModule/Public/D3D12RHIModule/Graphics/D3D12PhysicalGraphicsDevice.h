#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

#include <RHIModule/Graphics/PhysicalGraphicsDevice.h>

namespace Volt::RHI
{
	class D3D12PhysicalGraphicsDevice final : public PhysicalGraphicsDevice
	{
	public:
		D3D12PhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer);
		~D3D12PhysicalGraphicsDevice() override;

		VT_NODISCARD VT_INLINE StringView GetDeviceName() const override { return ""; }
		VT_NODISCARD VT_INLINE const DeviceVendor GetDeviceVendor() const override { return m_vendor; }

	protected:
		void* GetHandleImpl() const override;

	private:
		ComPtr<IDXGIAdapter4> FindBestSuitableDevice(bool enableDebugLayer);

		std::string m_name;
		DeviceVendor m_vendor;
		ComPtr<IDXGIAdapter4> m_adapter = nullptr;
	};
}
