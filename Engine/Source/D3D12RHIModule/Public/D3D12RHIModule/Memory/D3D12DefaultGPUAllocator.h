#pragma once

#include "D3D12RHIModule/Memory/D3D12Allocation.h"

#include <RHIModule/Memory/GPUAllocator.h>
#include <RHIModule/Memory/AllocationCache.h>

#include <CoreUtilities/Allocators/FixedSizeArenaAllocator.h>

namespace D3D12MA
{
	class Allocator;
}

namespace Volt::RHI
{
	class D3D12DefaultGPUAllocator : public DefaultGPUAllocator
	{
	public:
		D3D12DefaultGPUAllocator();
		~D3D12DefaultGPUAllocator() override;

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
		void DestroyBufferInternal(Handle<Allocation> allocation);
		void DestroyImageInternal(Handle<Allocation> allocation);

		D3D12MA::Allocator* m_allocator;

		AllocationCache m_allocationCache{};

		FixedSizeArenaAllocator<D3D12BufferAllocation> m_bufferAllocationArena;
		FixedSizeArenaAllocator<D3D12ImageAllocation> m_imageAllocationArena;
	};
}
