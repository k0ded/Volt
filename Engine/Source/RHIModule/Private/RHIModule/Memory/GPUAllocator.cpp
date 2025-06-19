#include "rhipch.h"

#include "RHIModule/Memory/GPUAllocator.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<TransientGPUAllocator> TransientGPUAllocator::Create()
	{
		return RHIModule::GetInstance().CreateTransientAllocator();
	}

	RefPtr<DefaultGPUAllocator> DefaultGPUAllocator::Create()
	{
		return RHIModule::GetInstance().CreateDefaultAllocator();
	}
}
