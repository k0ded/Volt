#pragma once

#include "RHIModule/Core/RHIInterface.h"

namespace Volt::RHI
{
	class VTRHI_API Fence : public RHIInterface
	{
	public:
		static RefPtr<Fence> Create();
		virtual void WaitUntilSignaled() const = 0;
		virtual bool IsSignaled() const = 0;
		virtual void Reset() = 0;
	};
}
