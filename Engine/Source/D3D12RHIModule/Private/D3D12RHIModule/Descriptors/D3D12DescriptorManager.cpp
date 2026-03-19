#include "dxpch.h"

#include "D3D12RHIModule/Descriptors/D3D12DescriptorManager.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include <RHIModule/RHICapabilities.h>

namespace Volt::RHI
{
	D3D12DescriptorManager g_descriptorManager;

	D3D12DescriptorManager::D3D12DescriptorManager()
	{
	}

	D3D12DescriptorManager::~D3D12DescriptorManager()
	{
	}

	D3D12DescriptorPointer D3D12DescriptorManager::Allocate(D3D12DescriptorType descriptorType)
	{
		return m_descriptorHeaps.at(descriptorType)->Allocate();
	}

	void D3D12DescriptorManager::Free(D3D12DescriptorType descriptorType, D3D12DescriptorPointer descriptorPointer)
	{
		m_descriptorHeaps.at(descriptorType)->Free(descriptorPointer);
	}

	void D3D12DescriptorManager::Initialize()
	{
		VT_ENSURE_MSG(m_isInitialized == false, "Should only be initialized once!");
		m_isInitialized = true;

		constexpr uint32_t NumDescriptors = 16384u;

		DescriptorHeapDesc desc{};
		desc.maxDescriptorCount = NumDescriptors;
		desc.supportsGPUDescriptors = false;

		{
			desc.descriptorType = D3D12DescriptorType::RTV;
			m_descriptorHeaps[D3D12DescriptorType::RTV] = CreateUnique<D3D12DescriptorHeap>(desc);
		}
		{
			desc.descriptorType = D3D12DescriptorType::DSV;
			m_descriptorHeaps[D3D12DescriptorType::DSV] = CreateUnique<D3D12DescriptorHeap>(desc);
		}
		{
			desc.descriptorType = D3D12DescriptorType::Sampler;
			m_descriptorHeaps[D3D12DescriptorType::Sampler] = CreateUnique<D3D12DescriptorHeap>(desc);
		}
		{
			desc.descriptorType = D3D12DescriptorType::CBV_SRV_UAV;
			m_descriptorHeaps[D3D12DescriptorType::CBV_SRV_UAV] = CreateUnique<D3D12DescriptorHeap>(desc);
		}
		{
			m_descriptorStack = CreateUnique<D3D12DescriptorStack>(D3D12DescriptorType::CBV_SRV_UAV);
			m_samplerDescriptorStack = CreateUnique<D3D12DescriptorStack>(D3D12DescriptorType::Sampler);
		}
	}

	void D3D12DescriptorManager::Shutdown()
	{
		m_descriptorHeaps.clear();
	}

	D3D12DescriptorPointer D3D12DescriptorManager::AllocateOnStack(D3D12DescriptorType descriptorType, uint32_t numDescriptors)
	{
		if (descriptorType == D3D12DescriptorType::CBV_SRV_UAV)
		{
			return m_descriptorStack->AllocateDescriptorRange(numDescriptors);
		}
		else
		{
			return m_samplerDescriptorStack->AllocateDescriptorRange(numDescriptors);
		}
	}

	void D3D12DescriptorManager::BeginFrame()
	{
		m_descriptorStack->BeginFrame();
		m_samplerDescriptorStack->BeginFrame();
	}

	uint64_t D3D12DescriptorManager::GetDescriptorSize(D3D12DescriptorType descriptorType) const
	{
		const auto& deviceProperties = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDeviceProperties();

		switch (descriptorType)
		{
			case D3D12DescriptorType::RTV:
				return deviceProperties.rtvDescriptorSize;
			case D3D12DescriptorType::DSV:
				return deviceProperties.dsvDescriptorSize;
			case D3D12DescriptorType::CBV_SRV_UAV:
				return deviceProperties.cbvSrvUavDescriptorSize;
			case D3D12DescriptorType::Sampler:
				return deviceProperties.samplerDescriptorSize;
		}

		VT_ENSURE(false);
		return 0;
	}
}
