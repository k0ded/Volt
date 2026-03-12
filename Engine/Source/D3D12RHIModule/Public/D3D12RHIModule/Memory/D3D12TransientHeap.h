#pragma once

#include "D3D12RHIModule/Memory/D3D12Allocation.h"

#include <RHIModule/Memory/TransientHeap.h>
#include <RHIModule/Memory/Allocation.h>

#include <CoreUtilities/Allocators/FixedSizeArenaAllocator.h>

struct ID3D12Heap;

namespace Volt::RHI
{
	class D3D12TransientHeap : public TransientHeap
	{
	public:
		D3D12TransientHeap(const TransientHeapCreateInfo& info);
		~D3D12TransientHeap() override;

		Handle<Allocation> CreateBuffer(const TransientBufferCreateInfo& createInfo, const std::string& name) override;
		Handle<Allocation> CreateImage(const TransientImageCreateInfo& createInfo, const std::string& name) override;

		void ForfeitBuffer(Handle<Allocation> allocation) override;
		void ForfeitImage(Handle<Allocation> allocation) override;

		const bool IsAllocationSupported(const uint64_t size, TransientHeapFlags heapFlags) const override;
		const UUID64 GetHeapID() const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		std::pair<uint32_t, AllocationBlock> FindNextAvailableBlock(const uint64_t size);
		void ForfeitAllocationBlock(const AllocationBlock& allocBlock);

		const bool IsAllocationSupportedInPage(const PageAllocation& page, const uint64_t size) const;

		void InitializeAsBufferHeap();
		void InitializeAsImageHeap();

		TransientHeapCreateInfo m_createInfo;
		MemoryRequirement m_memoryRequirements;

		std::array<PageAllocation, MAX_PAGE_COUNT> m_pageAllocations;

		std::mutex m_allocationMutex;
		UUID64 m_heapId;

		FixedSizeArenaAllocator<D3D12TransientBufferAllocation> m_bufferAllocationArena;
		FixedSizeArenaAllocator<D3D12TransientImageAllocation> m_imageAllocationArena;
	};
}
