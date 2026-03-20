#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

#include <CoreUtilities/Containers/VectorVariants.h>

namespace Volt::RHI
{
	class CommandBuffer;
	class Semaphore;
	class Fence;
	class Fence;

	struct DeviceQueueExecuteInfo
	{
		InlineVector<IntRef<CommandBuffer>, 1> commandBuffers;
		InlineVector<IntRef<Fence>, 1> signalFences;
	
		IntRef<Fence> executionFence;
	};

	class VTRHI_API DeviceQueue : public RHIInterface
	{
	public:
		VT_DELETE_COPY_MOVE(DeviceQueue);

		virtual void WaitForQueue() = 0;
		virtual void Execute(const DeviceQueueExecuteInfo& executeInfo) = 0;

	protected:
		DeviceQueue() = default;
	};
}
