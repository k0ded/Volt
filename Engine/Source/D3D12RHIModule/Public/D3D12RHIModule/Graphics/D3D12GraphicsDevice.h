#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

#include <RHIModule/Graphics/GraphicsDevice.h>

struct ID3D12Device10;
struct ID3D12DebugDevice;

namespace Volt::RHI
{
	class D3D12GraphicsDevice final : public GraphicsDevice
	{
	public:
		struct Properties
		{
			uint32_t rtvDescriptorSize;
			uint32_t dsvDescriptorSize;
			uint32_t cbvSrvUavDescriptorSize;
			uint32_t samplerDescriptorSize;
		};

		struct Capabilities
		{
			bool supportsEnhancedBarriers = false;
		};

		D3D12GraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer);
		~D3D12GraphicsDevice() override;

		RefPtr<DeviceQueue> GetDeviceQueue(QueueType queueType) const override;

		VT_NODISCARD VT_INLINE ID3D12Device10* GetDevice10() const { return m_device.Get(); }
		VT_NODISCARD VT_INLINE const Properties& GetDeviceProperties() const { return m_properties; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void InitializeProperties();
		void InitializeCapabilities();

		Properties m_properties;
		Capabilities m_capabilities;

		Map<QueueType, RefPtr<DeviceQueue>> m_deviceQueues;

		ComPtr<ID3D12Device10> m_device;
		
#ifdef VT_ENABLE_VALIDATION
		ComPtr<ID3D12DebugDevice> m_debugDevice;
#endif
	};
}
