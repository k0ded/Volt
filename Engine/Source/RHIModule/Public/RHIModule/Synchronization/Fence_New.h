#pragma once

#include "RHIModule/Core/RHIInterface.h"

namespace Volt::RHI
{
	class VTRHI_API Fence_New : public RHIInterface
	{
	public:
		static RefPtr<Fence_New> Create();
		virtual void WaitUntilSignaled() const = 0;
		virtual bool IsSignaled() const = 0;
	};
}
