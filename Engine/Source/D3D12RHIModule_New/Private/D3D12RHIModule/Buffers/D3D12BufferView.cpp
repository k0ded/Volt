#include "dxpch.h"

#include "D3D12RHIModule/Buffers/D3D12BufferView.h"
#include "D3D12RHIModule/Descriptors/D3D12DescriptorManager.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/RHICapabilities.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/EnumUtils.h>
#include <CoreUtilities/MemoryUtility.h>

namespace Volt::RHI
{

	D3D12BufferView::D3D12BufferView(const BufferViewDesc& desc, RawPtr<StorageBuffer> buffer)
		: m_desc(desc), m_resource(buffer)
	{
		m_viewType = D3D12ViewType::SRV;
		CreateSRV();

		if (EnumValueContainsFlag(buffer->GetDesc().memoryUsage, MemoryUsage::GPU))
		{
			m_viewType |= D3D12ViewType::UAV;
			CreateUAV();
		}
	}

	D3D12BufferView::D3D12BufferView(const BufferViewDesc& desc, RawPtr<UniformBuffer> buffer)
		: m_desc(desc), m_resource(buffer)
	{
		m_viewType = D3D12ViewType::CBV;
		CreateCBV();
	}

	D3D12BufferView::~D3D12BufferView()
	{
		RHIModule::GetInstance().DestroyResource([srvDescriptor = m_srvDescriptor, uavDescriptor = m_uavDescriptor, cbvDescriptor = m_cbvDescriptor]()
		{
			if (srvDescriptor.IsValid())
			{
				g_descriptorManager.Free(D3D12DescriptorType::CBV_SRV_UAV, srvDescriptor);
			}

			if (uavDescriptor.IsValid())
			{
				g_descriptorManager.Free(D3D12DescriptorType::CBV_SRV_UAV, uavDescriptor);
			}

			if (cbvDescriptor.IsValid())
			{
				g_descriptorManager.Free(D3D12DescriptorType::CBV_SRV_UAV, cbvDescriptor);
			}
		});

		m_srvDescriptor.Reset();
		m_uavDescriptor.Reset();
		m_cbvDescriptor.Reset();
	}

	const uint64_t D3D12BufferView::GetDeviceAddress() const
	{
		return m_resource->GetDeviceAddress();
	}

	bool D3D12BufferView::IsTexelBufferView() const
	{
		return m_desc.bufferFormat != RHI::PixelFormat::UNDEFINED;
	}

	void* D3D12BufferView::GetHandleImpl() const
	{
		return m_resource->GetHandle<void*>();
	}

	void D3D12BufferView::CreateSRV()
	{
		VT_ENSURE(m_resource->GetType() == ResourceType::StorageBuffer);

		D3D12_BUFFER_SRV_FLAGS flags = D3D12_BUFFER_SRV_FLAG_NONE;
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};

		const BufferDesc& desc = m_resource->AsRef<StorageBuffer>().GetDesc();
		const uint32_t numElements = m_desc.size == std::numeric_limits<size_t>::max() ? desc.count : static_cast<uint32_t>(m_desc.size / desc.elementSize);

		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		viewDesc.Format = IsTexelBufferView() ? ConvertFormatToD3D12Format(m_desc.bufferFormat) : DXGI_FORMAT_UNKNOWN;
		viewDesc.Buffer.FirstElement = m_desc.offset / desc.elementSize;
		viewDesc.Buffer.Flags = flags;
		viewDesc.Buffer.NumElements = numElements;
		viewDesc.Buffer.StructureByteStride = IsTexelBufferView() ? 0 : static_cast<uint32_t>(desc.elementSize);

		m_srvDescriptor = g_descriptorManager.Allocate(D3D12DescriptorType::CBV_SRV_UAV);

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();
		d3d12Device->CreateShaderResourceView(m_resource->GetHandle<ID3D12Resource*>(), &viewDesc, D3D12_CPU_DESCRIPTOR_HANDLE(m_srvDescriptor.GetCPUPointer()));
	}

	void D3D12BufferView::CreateUAV()
	{
		VT_ENSURE(m_resource->GetType() == ResourceType::StorageBuffer);

		D3D12_BUFFER_UAV_FLAGS flags = D3D12_BUFFER_UAV_FLAG_NONE;
		D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};

		const BufferDesc& desc = m_resource->AsRef<StorageBuffer>().GetDesc();
		const uint32_t numElements = m_desc.size == std::numeric_limits<size_t>::max() ? desc.count : static_cast<uint32_t>(m_desc.size / desc.elementSize);

		viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		viewDesc.Format = IsTexelBufferView() ? ConvertFormatToD3D12Format(m_desc.bufferFormat) : DXGI_FORMAT_UNKNOWN;
		viewDesc.Buffer.CounterOffsetInBytes = 0;
		viewDesc.Buffer.Flags = flags;
		viewDesc.Buffer.FirstElement = m_desc.offset / desc.elementSize;
		viewDesc.Buffer.NumElements = numElements;
		viewDesc.Buffer.StructureByteStride = IsTexelBufferView() ? 0 : static_cast<uint32_t>(desc.elementSize);

		m_uavDescriptor = g_descriptorManager.Allocate(D3D12DescriptorType::CBV_SRV_UAV);

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();
		d3d12Device->CreateUnorderedAccessView(m_resource->GetHandle<ID3D12Resource*>(), nullptr, &viewDesc, D3D12_CPU_DESCRIPTOR_HANDLE(m_uavDescriptor.GetCPUPointer()));
	}

	void D3D12BufferView::CreateCBV()
	{
		const uint32_t bufferSize = m_resource->AsRef<UniformBuffer>().GetSize();
		uint64_t actualSize = m_desc.size == std::numeric_limits<size_t>::max() ? bufferSize : m_desc.size;
		actualSize = std::min(actualSize, g_rhiCapabilities.maxUniformBufferViewSize);

		actualSize = ::Utility::Align(actualSize, g_rhiCapabilities.minUniformBufferAlignment);

		D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc{};
		viewDesc.BufferLocation = GetDeviceAddress() + m_desc.offset;
		viewDesc.SizeInBytes = static_cast<uint32_t>(actualSize);

		m_cbvDescriptor = g_descriptorManager.Allocate(D3D12DescriptorType::CBV_SRV_UAV);

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();
		d3d12Device->CreateConstantBufferView(&viewDesc, D3D12_CPU_DESCRIPTOR_HANDLE(m_cbvDescriptor.GetCPUPointer()));
	}
}
