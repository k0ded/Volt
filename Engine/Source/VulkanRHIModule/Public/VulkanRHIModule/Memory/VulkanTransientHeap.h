#pragma once

#include <RHIModule/Memory/TransientHeap.h>

struct VkDeviceMemory_T;

namespace Volt::RHI
{
	class VulkanTransientHeap : public TransientHeap
	{
	public:
		VulkanTransientHeap(const TransientHeapCreateInfo& info);
		~VulkanTransientHeap() override;
	
		void ReservePages(uint32_t numPages) override;

		VkDeviceMemory_T* GetPageMemoryHandle(uint32_t pageIndex);

	protected:
		void* GetHandleImpl() const override;
	
	private:
		struct MemoryPage
		{
			VkDeviceMemory_T* memoryHandle;
			uint64_t pageSize;
		};

		void InitializeAsBufferHeap();
		void InitializeAsImageHeap();

		void AllocateNewPage(uint64_t minSize);

		MemoryRequirement m_memoryRequirements;
		TransientHeapCreateInfo m_createInfo;
	
		Vector<MemoryPage> m_memoryPages;
	};
}
