#pragma once

#include <RHIModule/RayTracing/AccelerationStructure.h>

struct VkAccelerationStructureKHR_T;

namespace Volt::RHI
{
	class StorageBuffer;
	class VulkanAccelerationStructure : public AccelerationStructure
	{
	public:
		VulkanAccelerationStructure(const AccelerationStructureCreateInfo& createInfo);
		~VulkanAccelerationStructure() override;

		uint64_t GetDeviceAddress() const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		void InitializeFromInfo(const AccelerationStructureCreateInfo& createInfo);
	
		RefPtr<StorageBuffer> m_backingBuffer;
		VkAccelerationStructureKHR_T* m_handle = nullptr;
		uint64_t m_deviceAddress = 0;
	};
}
