#include "rhipch.h"

#include "RHIModule/RayTracing/AccelerationStructure.h"
#include "RHIModule/RHIProxy.h"

namespace Volt::RHI
{
	RefPtr<AccelerationStructure> AccelerationStructure::Create(const AccelerationStructureCreateInfo& createInfo)
	{
		return RHIProxy::GetInstance().CreateAccelerationStructure(createInfo);
	}
}
