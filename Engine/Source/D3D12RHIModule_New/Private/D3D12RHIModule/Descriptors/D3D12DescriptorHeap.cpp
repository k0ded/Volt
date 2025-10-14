#include "dxpch.h"

#include "D3D12RHIModule/Descriptors/D3D12DescriptorHeap.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include <RHIModule/RHIModule.h>

namespace Volt::RHI
{
	D3D12DescriptorHeap::D3D12DescriptorHeap(const DescriptorHeapDesc& desc)
	{
		VT_ENSURE(desc.descriptorType != D3D12DescriptorType::None && "Invalid descriptor type!");
		VT_ENSURE(desc.maxDescriptorCount > 0);

		m_desc = desc;

		const auto& deviceProperties = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDeviceProperties();

		switch (m_desc.descriptorType)
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
				break;
		}

		AllocateHeap(m_desc.maxDescriptorCount);
		m_freeDescriptors.Allocate(m_desc.maxDescriptorCount);
		m_numCurrentDescriptors = 0;
	}

	D3D12DescriptorHeap::~D3D12DescriptorHeap()
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

	D3D12DescriptorPointer D3D12DescriptorHeap::Allocate()
	{
		D3D12DescriptorPointer result;
		if (m_freeDescriptors.Pop(result))
		{
			return result;
		}

		const uint64_t descriptorIndex = m_numCurrentDescriptors.fetch_add(1u, std::memory_order::relaxed);
		const uint64_t descriptorOffset = descriptorIndex * m_descriptorSize;

		result.cpuPointer = m_startPointer.cpuPointer + descriptorOffset;
		if (m_desc.supportsGPUDescriptors)
		{
			result.gpuPointer = m_startPointer.gpuPointer + descriptorOffset;
		}

		return result;
	}

	void D3D12DescriptorHeap::Free(D3D12DescriptorPointer descriptor)
	{
		m_freeDescriptors.Push(descriptor);
	}

	void D3D12DescriptorHeap::AllocateHeap(uint32_t maxDescriptorCount)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = Utility::GetDescriptorType(m_desc.descriptorType);
		desc.Flags = m_desc.supportsGPUDescriptors ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NumDescriptors = maxDescriptorCount;
		desc.NodeMask = 0;

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDevice10();
		VT_D3D12_CHECK(d3d12Device->CreateDescriptorHeap(&desc, VT_D3D12_ID(m_descriptorHeap)));

		m_startPointer.cpuPointer = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;

		if (m_desc.supportsGPUDescriptors)
		{
			m_startPointer.gpuPointer = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
		}
	}
}
