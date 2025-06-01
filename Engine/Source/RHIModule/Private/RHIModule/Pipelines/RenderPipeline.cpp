#include "rhipch.h"

#include "RHIModule/Pipelines/RenderPipeline.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<RenderPipeline> RenderPipeline::Create2(const RenderPipelineCreateInfo& createInfo)
	{
		return RHIModule::GetInstance().CreateRenderPipeline2(createInfo);
	}
}
