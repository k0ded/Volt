#include "dxpch.h"

#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"
#include "D3D12RHIModule/Graphics/D3D12DeviceQueue.h"

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>

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

		m_deviceQueues[QueueType::Graphics] = IntRef<D3D12DeviceQueue>::Create(DeviceQueueCreateInfo{ this, QueueType::Graphics });
		m_deviceQueues[QueueType::TransferCopy] = IntRef<D3D12DeviceQueue>::Create(DeviceQueueCreateInfo{ this, QueueType::TransferCopy });
		m_deviceQueues[QueueType::Compute] = IntRef<D3D12DeviceQueue>::Create(DeviceQueueCreateInfo{ this, QueueType::Compute });
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

	IntRef<DeviceQueue> D3D12GraphicsDevice::GetDeviceQueue(QueueType queueType) const
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

	uint64_t D3D12GraphicsDevice::GetMaxRequiredStagingBufferSizeForImage(RawPtr<Image> image) const
	{
		const ImageDesc& desc = image->GetDesc();

		const uint32_t numSubresources = desc.mips * desc.layers;

		Vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numSubresources);
		Vector<uint32_t> numRows(numSubresources);
		Vector<uint64_t> rowSizeInBytes(numSubresources);

		uint64_t requiredSize;

		const D3D12_RESOURCE_DESC d3d12Desc = image->GetHandle<ID3D12Resource*>()->GetDesc();

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDevice10();
		d3d12Device->GetCopyableFootprints(
			&d3d12Desc,
			0,
			numSubresources,
			0,
			layouts.data(),
			numRows.data(),
			rowSizeInBytes.data(),
			&requiredSize
		);

		return requiredSize;
	}

	uint64_t D3D12GraphicsDevice::GetRowPitchForWidth(RawPtr<Image> image, uint32_t width) const
	{
		const ImageDesc& desc = image->GetDesc();
		uint64_t rowPitch = width * RHI::Utility::GetByteSizePerPixelFromFormat(desc.format);
		rowPitch = (rowPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);

		return rowPitch;
	}
}
