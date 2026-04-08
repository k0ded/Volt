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

		/*
		* A (binary) semaphore can only be waited on once, so this function
		* tries to assign the current caller as the waiter and sets the
		* semaphore to not waitable.
		*/
		bool TryGetWait();

		void ResetWait();

	protected:
		void* GetHandleImpl() const override;

	private:
		VkSemaphore_T* m_semaphore = nullptr;
		std::atomic_bool m_isWaitable = true;
	};
}
