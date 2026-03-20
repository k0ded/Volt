#include "rhipch.h"

#include "RHIModule/RayTracing/ShaderBindingTable.h"
#include "RHIModule/Pipelines/RayTracingPipeline.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<ShaderBindingTable> ShaderBindingTable::Create(IntRef<RayTracingPipeline> pipeline)
	{
		return RHIModule::GetInstance().CreateShaderBindingTable(pipeline);
	}
}
