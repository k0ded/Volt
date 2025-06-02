#include "rhipch.h"

#include "RHIModule/Pipelines/ComputePipeline.h"
#include "RHIModule/RHIModule.h"
#include "RHIModule/Shader/Shader.h"

namespace Volt::RHI
{
	RefPtr<ComputePipeline> ComputePipeline::Create(RefPtr<Shader> shader, bool useGlobalResources)
	{
		return RHIModule::GetInstance().CreateComputePipeline(shader, useGlobalResources);
	}

	RefPtr<ComputePipeline> ComputePipeline::Create(RefPtr<Shader2> shader)
	{
		return RHIModule::GetInstance().CreateComputePipeline(shader);
	}

}
