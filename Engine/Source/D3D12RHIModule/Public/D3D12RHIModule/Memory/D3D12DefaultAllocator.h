#pragma once

#include "D3D12RHIModule/Memory/D3D12Allocation.h"

#include <RHIModule/Memory/Allocator.h>
#include <RHIModule/Memory/AllocationCache.h>

#include <CoreUtilities/Allocators/ArenaAllocator.h>

namespace D3D12MA
{
	class Allocator;
}

namespace Volt::RHI
{
	class D3D12DefaultAllocator : public DefaultAllocator
	{
	public:
		D3D12DefaultAllocator();
		~D3D12DefaultAllocator() override;

		Handle<Allocation> CreateBuffer(const size_t size, BufferUsage usage, MemoryUsage memoryUsage, const std::string& name) override;
		Handle<Allocation> CreateImage(const ImageSpecification& imageSpecification, MemoryUsage memoryUsage) override;

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

		std::mutex m_bufferAllocationMutex;
		std::mutex m_imageAllocationMutex;

		ArenaAllocator<D3D12BufferAllocation, 5000> m_bufferAllocationArena;
		ArenaAllocator<D3D12ImageAllocation, 5000> m_imageAllocationArena;
	};
}
