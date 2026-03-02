#pragma once

#include "RHIModule/Graphics/DeviceQueue.h"

#include <thread>

namespace Volt::RHI
{
	class RHISubmissionThread
	{
	public:
		virtual ~RHISubmissionThread() = default;

		virtual void QueueSubmit(DeviceQueueExecuteInfo&& executeInfo, QueueType queueType) = 0;
		virtual std::thread::id GetSubmissionThreadId() const = 0;

	protected:
		RHISubmissionThread() = default;
	};
}
