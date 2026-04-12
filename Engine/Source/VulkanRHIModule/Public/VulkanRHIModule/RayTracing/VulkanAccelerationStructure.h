#pragma once

#include "VulkanRHIModule/LastSubmissionTracker.h"

#include <RHIModule/RayTracing/AccelerationStructure.h>

struct VkAccelerationStructureKHR_T;

namespace Volt::RHI
{
	class Buffer;
	class VulkanAccelerationStructure final : public AccelerationStructure, public LastSubmissionTracker
	{
	public:
		VulkanAccelerationStructure(const AccelerationStructureCreateInfo& createInfo);
		~VulkanAccelerationStructure() override;

		uint64_t GetDeviceAddress() const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		void InitializeFromInfo(const AccelerationStructureCreateInfo& createInfo);
	
		IntRef<Buffer> m_backingBuffer;
		VkAccelerationStructureKHR_T* m_handle = nullptr;
		uint64_t m_deviceAddress = 0;
	};
}
