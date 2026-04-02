#pragma once

#include <RHIModule/Synchronization/Semaphore.h>

struct VkSemaphore_T;

namespace Volt::RHI
{
	class VulkanSemaphore : public Semaphore
	{
	public:
		VulkanSemaphore();
		~VulkanSemaphore() override;

	protected:
		void* GetHandleImpl() const override;

	private:
		VkSemaphore_T* m_semaphore = nullptr;
	};
}
