#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

#include <RHIModule/Graphics/DeviceQueue.h>

struct ID3D12CommandQueue;

namespace Volt::RHI
{
	class D3D12DeviceQueue final : public DeviceQueue
	{
	public:
		D3D12DeviceQueue(const DeviceQueueCreateInfo& createInfo);
		~D3D12DeviceQueue() override;

		void WaitForQueue() override;
		void Execute(const DeviceQueueExecuteInfo& commandBuffer) override;

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateCommandQueue(GraphicsDevice* graphicsDevice);
		void CreateQueueFence(GraphicsDevice* graphicsDevice);

		std::mutex m_executeMutex{};

		ComPtr<ID3D12Fence> m_fence;
		void* m_windowsFenceEvent = nullptr;

		ComPtr<ID3D12CommandQueue> m_commandQueue;
		QueueType m_queueType;
		uint64_t m_semaphoreValue = 1;
	};
}
