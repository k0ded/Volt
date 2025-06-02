#pragma once

#include "VulkanRHIModule/Core.h"
#include "VulkanRHIModule/Memory/VulkanAllocation.h"

#include <RHIModule/Memory/GPUAllocator.h>
#include <RHIModule/Memory/AllocationCache.h>

#include <CoreUtilities/Allocators/ArenaAllocator.h>

struct VmaAllocator_T;

namespace Volt::RHI
{
	class VulkanDefaultGPUAllocator : public DefaultGPUAllocator
	{
	public:
		VulkanDefaultGPUAllocator();
		~VulkanDefaultGPUAllocator() override;

		Handle<Allocation> CreateBuffer(const BufferDesc& desc) override;
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

		VmaAllocator_T* m_allocator = nullptr;

		AllocationCache m_allocationCache{};

		std::mutex m_imageAllocationMutex;
		std::mutex m_bufferAllocationMutex;

		ArenaAllocator<VulkanBufferAllocation, 5000> m_bufferAllocationArena;
		ArenaAllocator<VulkanImageAllocation, 5000> m_imageAllocationArena;
	};
}
