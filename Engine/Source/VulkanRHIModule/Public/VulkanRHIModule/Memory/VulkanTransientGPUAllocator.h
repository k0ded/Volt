#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Memory/GPUAllocator.h>
#include <RHIModule/Memory/AllocationCache.h>
#include <RHIModule/Memory/TransientHeap.h>

namespace Volt::RHI
{
	class VulkanTransientGPUAllocator : public TransientGPUAllocator
	{
	public:
		VulkanTransientGPUAllocator();
		~VulkanTransientGPUAllocator() override;

		Handle<Allocation> CreateBuffer(const BufferDesc& desc) override;
		Handle<Allocation> CreateImage(const ImageDesc& imageSpecification, MemoryUsage memoryUsage) override;

		void DestroyBuffer(Handle<Allocation> allocation) override;
		void DestroyImage(Handle<Allocation> allocation) override;

		Vector<Handle<Allocation>> GetActiveBufferAllocations() const override;
		Vector<Handle<Allocation>> GetActiveImageAllocations() const override;

		void Update() override;

	protected:
		void* GetHandleImpl() const override;

	private:
		inline static constexpr uint32_t HEAP_PAGE_SIZE = 128 * 1024 * 1024;

		void CreateDefaultHeaps();

		void DestroyBufferInternal(Handle<Allocation> allocation);
		void DestroyImageInternal(Handle<Allocation> allocation);

		// There are called if their parent heap has been destroyed for some reason
		void DestroyOrphanBuffer(Handle<Allocation> allocation);
		void DestroyOrphanImage(Handle<Allocation> allocation);

		VT_NODISCARD RefPtr<TransientHeap> CreateNewImageHeap();
		VT_NODISCARD RefPtr<TransientHeap> CreateNewBufferHeap(TransientHeapFlags heapFlags);

		Vector<RefPtr<TransientHeap>> m_bufferHeaps;
		Vector<RefPtr<TransientHeap>> m_imageHeaps;

		AllocationCache m_allocationCache{};
	};
}
