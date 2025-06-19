#include "rhipch.h"

#include "RHIModule/RayTracing/AccelerationStructure.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<AccelerationStructure> AccelerationStructure::Create(const AccelerationStructureCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateAccelerationStructure(createInfo);
	}
}
