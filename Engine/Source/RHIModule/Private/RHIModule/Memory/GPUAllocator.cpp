#include "rhipch.h"

#include "RHIModule/Memory/GPUAllocator.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<DefaultGPUAllocator> DefaultGPUAllocator::Create()
	{
		return RHIModule::GetInstance().CreateDefaultAllocator();
	}
}
