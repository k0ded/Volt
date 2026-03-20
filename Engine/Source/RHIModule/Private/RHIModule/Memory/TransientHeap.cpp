#include "rhipch.h"

#include "RHIModule/Memory/TransientHeap.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<TransientHeap> TransientHeap::Create(const TransientHeapCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateTransientHeap(createInfo);
	}
}
