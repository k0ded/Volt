#include "dxpch.h"

#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"
#include "D3D12RHIModule/Graphics/D3D12DeviceQueue.h"

#include <RHIModule/RHICapabilities.h>

namespace Volt::RHI
{
	D3D12GraphicsDevice::D3D12GraphicsDevice(const GraphicsDeviceCreateInfo& info, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer)
	{
		VT_D3D12_CHECK(D3D12CreateDevice(physicalGraphicsDevice->GetHandle<IDXGIAdapter4*>(), D3D_FEATURE_LEVEL_11_0, VT_D3D12_ID(m_device)));

#ifdef VT_ENABLE_VALIDATION
		if (enableDebugLayer)
		{
			VT_D3D12_CHECK(m_device->QueryInterface(VT_D3D12_ID(m_debugDevice)));
		}
#endif

		InitializeProperties();
		InitializeCapabilities();

		m_deviceQueues[QueueType::Graphics] = RefPtr<D3D12DeviceQueue>::Create(DeviceQueueCreateInfo{ this, QueueType::Graphics });
		m_deviceQueues[QueueType::TransferCopy] = RefPtr<D3D12DeviceQueue>::Create(DeviceQueueCreateInfo{ this, QueueType::TransferCopy });
		m_deviceQueues[QueueType::Compute] = RefPtr<D3D12DeviceQueue>::Create(DeviceQueueCreateInfo{ this, QueueType::Compute });
	}

	D3D12GraphicsDevice::~D3D12GraphicsDevice()
	{
		m_deviceQueues[QueueType::Graphics].Reset();
		m_deviceQueues[QueueType::TransferCopy].Reset();
		m_deviceQueues[QueueType::Compute].Reset();

#ifdef VT_ENABLE_VALIDATION
		if (m_debugDevice)
		{
			m_debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
		}
#endif
	}

	RefPtr<DeviceQueue> D3D12GraphicsDevice::GetDeviceQueue(QueueType queueType) const
	{
		return m_deviceQueues.at(queueType);
	}

	void* D3D12GraphicsDevice::GetHandleImpl() const
	{
		VT_ENSURE_MSG(false, "Use GetDeviceN instead!");
		return nullptr;
	}

	void D3D12GraphicsDevice::InitializeProperties()
	{
		m_properties.rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_properties.dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_properties.cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_properties.samplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	void D3D12GraphicsDevice::InitializeCapabilities()
	{
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS12 options{};
			m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options, sizeof(options));

			m_capabilities.supportsEnhancedBarriers = options.EnhancedBarriersSupported;
		}

		g_rhiCapabilities.minUniformBufferAlignment = 256u;
	}
}
