#include <rhipch.h>

#include "RHIModule/Pipelines/RayTracingPipeline.h"
#include "RHIModule/RHIProxy.h"

namespace Volt::RHI
{
	RefPtr<RayTracingPipeline> RayTracingPipeline::Create(const RayTracingPipelineCreateInfo& createInfo)
	{
		return RHIProxy::GetInstance().CreateRayTracingPipeline(createInfo);
	}
}
