#pragma once

#include "RHIModule/Core/RHIInterface.h"

namespace Volt::RHI
{
	class Fence : public ArenaRHIInterface
	{
	public:
		VTRHI_API static IntRef<Fence> Create();
		virtual void WaitUntilSignaled() const = 0;
		virtual bool IsSignaled() const = 0;
		virtual void Reset() = 0;
	};
}
