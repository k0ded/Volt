#pragma once

#include "RHIModule/Core/RHIInterface.h"

namespace Volt::RHI
{
	class VTRHI_API Semaphore : public ArenaRHIInterface
	{
	public:
		 static IntRef<Semaphore> Create();

	protected:
		Semaphore() = default;
		~Semaphore() override = default;
	};
}
