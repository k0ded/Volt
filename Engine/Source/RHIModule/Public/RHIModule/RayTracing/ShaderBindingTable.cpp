#include "rhipch.h"

#include "RHIModule/RayTracing/ShaderBindingTable.h"
#include "RHIModule/Pipelines/RayTracingPipeline.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<ShaderBindingTable> ShaderBindingTable::Create(RefPtr<RayTracingPipeline> pipeline)
	{
		return RHIModule::GetInstance().CreateShaderBindingTable(pipeline);
	}
}
