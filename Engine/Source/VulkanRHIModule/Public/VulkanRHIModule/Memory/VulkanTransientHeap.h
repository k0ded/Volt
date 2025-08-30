#pragma once

#include "VulkanRHIModule/Memory/VulkanAllocation.h"

#include <RHIModule/Memory/TransientHeap.h>
#include <RHIModule/Memory/Allocation.h>

#include <CoreUtilities/Allocators/ArenaAllocator.h>

struct VkDeviceMemory_T;
struct VkImageCreateInfo;

namespace Volt::RHI
{
	class VulkanTransientHeap : public TransientHeap
	{
	public:
		VulkanTransientHeap(const TransientHeapCreateInfo& info);
		~VulkanTransientHeap() override;

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

		ArenaAllocator<VulkanTransientBufferAllocation> m_bufferAllocationArena;
		ArenaAllocator<VulkanTransientImageAllocation> m_imageAllocationArena;
	};
}
