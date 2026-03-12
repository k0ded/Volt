#pragma once

#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/RHICapabilities.h>

#include <CoreUtilities/Allocators/Handle.h>

#include <atomic>

namespace Volt::RHI
{
	class VulkanDescriptorHeap
	{
	public:
		VulkanDescriptorHeap();
		~VulkanDescriptorHeap();

		void BeginFrame();
		void Flush();

		uint64_t AllocateDescriptorSet(uint64_t descriptorSetLayoutSize);
		uint64_t GetDeviceAddress() const;
		uint64_t GetBaseOffset() const;
		uint8_t* GetHeapPointer();

	private:
		void Initialize();
		void Release();

		Handle<Allocation> m_descriptorHeapAllocation;

		struct RingBufferState
		{
			uint8_t* mappedPtr;
			uint64_t size;
			std::atomic_uint64_t head;
		};

		RingBufferState m_ringBufferState;
		uint32_t m_frameIndex = 0;
	};
}
