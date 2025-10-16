#include "dxpch.h"

#include "D3D12RHIModule/Descriptors/D3D12DescriptorStack.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include "RHIModule/RHICapabilities.h"

#include <RHIModule/RHIModule.h>

namespace Volt::RHI
{
	constexpr uint64_t DescriptorRingBufferSize = (1ull << 18);

	D3D12DescriptorStack::D3D12DescriptorStack(D3D12DescriptorType descriptorType)
	{
		Initialize(descriptorType);
	}

	D3D12DescriptorStack::~D3D12DescriptorStack()
	{
		Release();
	}

	D3D12DescriptorPointer D3D12DescriptorStack::AllocateDescriptorRange(uint32_t numDescriptors)
	{
		D3D12DescriptorPointer result;

		const uint64_t descriptorStartIndex = m_ringBufferState.head.fetch_add(numDescriptors, std::memory_order::relaxed);

		result.cpuPointer = m_startPointer.cpuPointer + m_ringBufferState.offset + descriptorStartIndex * m_descriptorSize;
		result.gpuPointer = m_startPointer.gpuPointer + m_ringBufferState.offset + descriptorStartIndex * m_descriptorSize;

		return result;
	}

	void D3D12DescriptorStack::BeginFrame()
	{
		m_frameIndex = (m_frameIndex + 1) % RHICapabilities::NumFramesInFlight;
		m_ringBufferState.head = 0;
		m_ringBufferState.offset = (m_stackSize * m_descriptorSize) * m_frameIndex;
	}

	void D3D12DescriptorStack::Initialize(D3D12DescriptorType descriptorType)
	{
		const auto& deviceProperties = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDeviceProperties();

		m_stackSize = DescriptorRingBufferSize;

		switch (descriptorType)
		{
			case D3D12DescriptorType::RTV:
				m_descriptorSize = deviceProperties.rtvDescriptorSize;
				break;
			case D3D12DescriptorType::DSV:
				m_descriptorSize = deviceProperties.dsvDescriptorSize;
				break;
			case D3D12DescriptorType::CBV_SRV_UAV:
				m_descriptorSize = deviceProperties.cbvSrvUavDescriptorSize;
				break;
			case D3D12DescriptorType::Sampler:
				m_descriptorSize = deviceProperties.samplerDescriptorSize;
				m_stackSize = 1000;
				break;
		}

		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = Utility::GetDescriptorType(descriptorType);
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NumDescriptors = static_cast<uint32_t>(m_stackSize * RHICapabilities::NumFramesInFlight);
		desc.NodeMask = 0;

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDevice10();
		VT_D3D12_CHECK(d3d12Device->CreateDescriptorHeap(&desc, VT_D3D12_ID(m_descriptorHeap)));

		m_startPointer.cpuPointer = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
		m_startPointer.gpuPointer = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
	}

	void D3D12DescriptorStack::Release()
	{
		if (!m_descriptorHeap)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([descriptorHeap = m_descriptorHeap]() mutable
		{
			descriptorHeap.Reset();
		});
	}
}
