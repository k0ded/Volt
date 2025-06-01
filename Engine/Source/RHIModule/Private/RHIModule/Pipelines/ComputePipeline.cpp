#include "rhipch.h"

#include "RHIModule/Pipelines/ComputePipeline.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<ComputePipeline> ComputePipeline::Create(RefPtr<Shader2> shader)
	{
		return RHIModule::GetInstance().CreateComputePipeline(shader);
	}

}
