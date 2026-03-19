#pragma once

#include "D3D12RHIModule/Descriptors/DescriptorCommon.h"
#include "D3D12RHIModule/Descriptors/D3D12DescriptorHeap.h"
#include "D3D12RHIModule/Descriptors/D3D12DescriptorStack.h"

namespace Volt::RHI
{
	class D3D12DescriptorManager
	{
	public:
		D3D12DescriptorManager();
		~D3D12DescriptorManager();

		void Initialize();
		void Shutdown();

		void BeginFrame();

		D3D12DescriptorPointer Allocate(D3D12DescriptorType descriptorType);
		void Free(D3D12DescriptorType descriptorType, D3D12DescriptorPointer descriptorPointer);

		D3D12DescriptorPointer AllocateOnStack(D3D12DescriptorType descriptorType, uint32_t numDescriptors);

		uint64_t GetDescriptorSize(D3D12DescriptorType descriptorType) const;

		VT_INLINE D3D12DescriptorStack& GetMainDescriptorStack() const { return *m_descriptorStack; }
		VT_INLINE D3D12DescriptorStack& GetSamplerDescriptorStack() const { return *m_samplerDescriptorStack; }

	private:
		Map<D3D12DescriptorType, Unique<D3D12DescriptorHeap>> m_descriptorHeaps;

		Unique<D3D12DescriptorStack> m_descriptorStack;
		Unique<D3D12DescriptorStack> m_samplerDescriptorStack;

		bool m_isInitialized = false;
	};

	extern D3D12DescriptorManager g_descriptorManager;
}
