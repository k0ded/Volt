#pragma once

#include "D3D12RHIModule/Descriptors/DescriptorCommon.h"
#include "D3D12RHIModule/Common/ComPtr.h"

namespace Volt::RHI
{
	class D3D12DescriptorStack
	{
	public:
		D3D12DescriptorStack(D3D12DescriptorType descriptorType);
		~D3D12DescriptorStack();

		void BeginFrame();

		D3D12DescriptorPointer AllocateDescriptorRange(uint32_t numDescriptors);
		VT_NODISCARD VT_INLINE ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() const { return m_descriptorHeap; }

	private:
		void Initialize(D3D12DescriptorType descriptorType);
		void Release();

		struct RingBufferState
		{
			uint64_t size;
			uint64_t offset;
			std::atomic_uint64_t head;
		};

		RingBufferState m_ringBufferState;
		uint32_t m_frameIndex = 0;
		uint32_t m_descriptorSize = 0;
		uint64_t m_stackSize = 0;

		D3D12DescriptorPointer m_startPointer{};
		ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
	};
}
