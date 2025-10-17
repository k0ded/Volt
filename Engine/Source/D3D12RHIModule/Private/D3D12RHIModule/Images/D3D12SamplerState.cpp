#include "dxpch.h"

#include "D3D12RHIModule/Images/D3D12SamplerState.h"
#include "D3D12RHIModule/Common/D3D12Helpers.h"
#include "D3D12RHIModule/Descriptors/D3D12DescriptorManager.h"

#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include <RHIModule/RHIModule.h>

namespace Volt::RHI
{
	D3D12SamplerState::D3D12SamplerState(const SamplerStateDesc& createInfo)
	{
		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.MaxAnisotropy = static_cast<uint32_t>(createInfo.anisotropyLevel);
		
		samplerDesc.Filter = Utility::VoltToD3D12Filter(createInfo.minFilter, createInfo.magFilter, createInfo.mipFilter, createInfo.compareOperator);
		samplerDesc.AddressU = Utility::VoltToD3D12WrapMode(createInfo.wrapMode);
		samplerDesc.AddressV = Utility::VoltToD3D12WrapMode(createInfo.wrapMode);
		samplerDesc.AddressW = Utility::VoltToD3D12WrapMode(createInfo.wrapMode);
		samplerDesc.MipLODBias = createInfo.mipLodBias;
		samplerDesc.MinLOD = createInfo.minLod;
		samplerDesc.MaxLOD = createInfo.maxLod;

		samplerDesc.BorderColor[0] = 1.f;
		samplerDesc.BorderColor[1] = 1.f;
		samplerDesc.BorderColor[2] = 1.f;
		samplerDesc.BorderColor[3] = 1.f;

		samplerDesc.MipLODBias = createInfo.mipLodBias;
		samplerDesc.MinLOD = createInfo.minLod;
		samplerDesc.MaxLOD = createInfo.maxLod;
		samplerDesc.ComparisonFunc = Utility::VoltToD3D12CompareOp(createInfo.compareOperator);

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();
		m_descriptor = g_descriptorManager.Allocate(D3D12DescriptorType::Sampler);

		d3d12Device->CreateSampler(&samplerDesc, D3D12_CPU_DESCRIPTOR_HANDLE(m_descriptor.GetCPUPointer()));
	}

	D3D12SamplerState::~D3D12SamplerState()
	{
		RHIModule::GetInstance().DestroyResource([descriptor = m_descriptor]()
		{
			g_descriptorManager.Free(D3D12DescriptorType::Sampler, descriptor);
		});

		m_descriptor.Reset();
	}

	void* D3D12SamplerState::GetHandleImpl() const
	{
		return nullptr;
	}
}
