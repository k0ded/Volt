#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

namespace Volt::RHI
{
	class CommandBuffer;
	class Semaphore;
	class Fence;
	class Fence_New;

	struct DeviceQueueExecuteInfo
	{
		Vector<RawPtr<CommandBuffer>> commandBuffers;
		Vector<RawPtr<Semaphore>> signalSemaphores;
	
		RawPtr<Fence_New> fence_new;
		RawPtr<Fence> fence;
	};

	class VTRHI_API DeviceQueue : public RHIInterface
	{
	public:
		VT_DELETE_COPY_MOVE(DeviceQueue);

		virtual void WaitForQueue() = 0;
		virtual void Execute(const DeviceQueueExecuteInfo& executeInfo) = 0;

		static RefPtr<DeviceQueue> Create(const DeviceQueueCreateInfo& createInfo);

	protected:
		DeviceQueue() = default;

		QueueType m_queueType = QueueType::Graphics;
	};
}
