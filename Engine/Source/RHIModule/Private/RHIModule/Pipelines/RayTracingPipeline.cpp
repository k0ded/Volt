#include <rhipch.h>

#include "RHIModule/Pipelines/RayTracingPipeline.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<RayTracingPipeline> RayTracingPipeline::Create(const RayTracingPipelineCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateRayTracingPipeline(createInfo);
	}
}
