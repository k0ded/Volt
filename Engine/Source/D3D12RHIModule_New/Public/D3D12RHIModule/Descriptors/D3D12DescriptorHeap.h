#pragma once

#include "D3D12RHIModule/Descriptors/DescriptorCommon.h"
#include "D3D12RHIModule/Common/ComPtr.h"

#include <CoreUtilities/Containers/AtomicStack.h>

struct ID3D12DescriptorHeap;

namespace Volt::RHI
{
	struct DescriptorHeapDesc
	{
		D3D12DescriptorType descriptorType;
		uint32_t maxDescriptorCount;
		bool supportsGPUDescriptors = false;
	};

	class D3D12DescriptorHeap
	{
	public:
		D3D12DescriptorHeap(const DescriptorHeapDesc& desc);
		~D3D12DescriptorHeap();

		D3D12DescriptorPointer Allocate();

		void Free(D3D12DescriptorPointer descriptor);


		VT_NODISCARD VT_INLINE ComPtr<ID3D12DescriptorHeap> GetHeap() const { return m_descriptorHeap; }

	private:
		void AllocateHeap(uint32_t maxDescriptorCount);

		DescriptorHeapDesc m_desc;
		uint32_t m_descriptorSize = 0;

		D3D12DescriptorPointer m_startPointer{};
		ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;

		std::atomic_uint64_t m_numCurrentDescriptors;
		AtomicStack<D3D12DescriptorPointer> m_freeDescriptors;
	};
}
