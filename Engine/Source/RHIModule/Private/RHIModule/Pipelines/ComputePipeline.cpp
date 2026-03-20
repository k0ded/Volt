#include "rhipch.h"

#include "RHIModule/Pipelines/ComputePipeline.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	IntRef<ComputePipeline> ComputePipeline::Create(IntRef<Shader> shader)
	{
		return RHIModule::GetInstance().CreateComputePipeline(shader);
	}

}
