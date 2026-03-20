#pragma once

#include <RHIModule/Memory/GPUAllocator.h>
#include <RHIModule/Memory/AllocationCache.h>

namespace Volt::RHI
{
	class TransientHeap;

	class D3D12TransientGPUAllocator : public TransientGPUAllocator
	{
	public:
		D3D12TransientGPUAllocator();
		~D3D12TransientGPUAllocator() override;

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

		VT_NODISCARD IntRef<TransientHeap> CreateNewImageHeap();
		VT_NODISCARD IntRef<TransientHeap> CreateNewBufferHeap();

		Vector<IntRef<TransientHeap>> m_bufferHeaps;
		Vector<IntRef<TransientHeap>> m_imageHeaps;

		AllocationCache m_allocationCache{};
	};
}
