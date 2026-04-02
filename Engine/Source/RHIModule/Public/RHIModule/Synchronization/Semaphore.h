#pragma once

#include "RHIModule/Core/RHIInterface.h"

namespace Volt::RHI
{
	class Semaphore : public ArenaRHIInterface
	{
	public:
		VTRHI_API static IntRef<Semaphore> Create();
	};
}
