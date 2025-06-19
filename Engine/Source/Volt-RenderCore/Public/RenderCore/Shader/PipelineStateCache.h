#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Pipelines/ComputePipeline.h>

namespace Volt
{
	class VTRC_API PipelineStateCache
	{
	public:
		PipelineStateCache();
		~PipelineStateCache();

		static RefPtr<RHI::RenderPipeline> GetRenderPipeline(const RHI::RenderPipelineCreateInfo& pipelineInfo);
		static RefPtr<RHI::ComputePipeline> GetComputePipeline(RefPtr<RHI::Shader> computeShader);

	private:
		inline static PipelineStateCache* s_instance = nullptr;

		Map<size_t, RefPtr<RHI::ComputePipeline>> m_computePipelineCache;
		Map<size_t, RefPtr<RHI::RenderPipeline>> m_renderPipelineCache;

		std::mutex m_computePipelineCacheMutex;
		std::mutex m_renderPipelineCacheMutex;
	};
}
