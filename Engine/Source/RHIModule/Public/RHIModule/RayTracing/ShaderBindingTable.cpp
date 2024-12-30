#include "rhipch.h"

#include "RHIModule/RayTracing/ShaderBindingTable.h"
#include "RHIModule/Pipelines/RayTracingPipeline.h"
#include "RHIModule/RHIProxy.h"

namespace Volt::RHI
{
	RefPtr<ShaderBindingTable> ShaderBindingTable::Create(RefPtr<RayTracingPipeline> pipeline)
	{
		return RHIProxy::GetInstance().CreateShaderBindingTable(pipeline);
	}
}
