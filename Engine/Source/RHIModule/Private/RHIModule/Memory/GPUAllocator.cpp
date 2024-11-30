#include "rhipch.h"

#include "RHIModule/Memory/GPUAllocator.h"
#include "RHIModule/RHIProxy.h"

namespace Volt::RHI
{
	RefPtr<TransientGPUAllocator> TransientGPUAllocator::Create()
	{
		return RHIProxy::GetInstance().CreateTransientAllocator();
	}

	RefPtr<DefaultGPUAllocator> DefaultGPUAllocator::Create()
	{
		return RHIProxy::GetInstance().CreateDefaultAllocator();
	}
}
