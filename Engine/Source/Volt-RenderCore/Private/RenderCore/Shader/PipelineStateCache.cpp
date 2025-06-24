#include "rcpch.h"

#include "RenderCore/Shader/PipelineStateCache.h"

#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	namespace Utility
	{
		inline static const size_t GetComputeShaderHash(RefPtr<RHI::Shader> shader)
		{
			return shader->GetHash();
		}

		inline static const size_t GetRenderPipelineHash(const RHI::RenderPipelineCreateInfo& pipelineInfo)
		{
			size_t hash = 0;
			for (const auto& shader : pipelineInfo.shaders)
			{
				hash = Math::HashCombine(hash, shader->GetHash());
			}

			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.topology)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.cullMode)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.fillMode)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.depthMode)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.depthCompareOperator)));
			hash = Math::HashCombine(hash, std::hash<bool>()(pipelineInfo.enablePrimitiveRestart));

			for (size_t i = 0; i < RHI::MAX_COLOR_ATTACHMENT_COUNT; ++i)
			{
				hash = Math::HashCombine(hash, std::hash<bool>()(pipelineInfo.attachmentBlendStates[i].enabled));
				if (pipelineInfo.attachmentBlendStates[i].enabled)
				{
					hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.attachmentBlendStates[i].srcColorBlend)));
					hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.attachmentBlendStates[i].dstColorBlend)));
					hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.attachmentBlendStates[i].colorBlendOp)));
					hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.attachmentBlendStates[i].srcAlphaBlend)));
					hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.attachmentBlendStates[i].dstAlphaBlend)));
					hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(pipelineInfo.attachmentBlendStates[i].alphaBlendOp)));
				}
			}

			return hash;
		}
	}

	PipelineStateCache::PipelineStateCache()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;
	}
	
	PipelineStateCache::~PipelineStateCache()
	{
		s_instance = nullptr;
	}

	RefPtr<RHI::RenderPipeline> PipelineStateCache::GetRenderPipeline(const RHI::RenderPipelineCreateInfo& pipelineInfo)
	{
		VT_PROFILE_FUNCTION();

		std::scoped_lock lock{ s_instance->m_renderPipelineCacheMutex };
		const size_t hash = Utility::GetRenderPipelineHash(pipelineInfo);

		if (s_instance->m_renderPipelineCache.contains(hash))
		{
			auto pipeline = s_instance->m_renderPipelineCache.at(hash);
			VT_ENSURE(pipeline->IsValid());

			return pipeline;
		}

		RefPtr<RHI::RenderPipeline> pipeline = RHI::RenderPipeline::Create(pipelineInfo);
		s_instance->m_renderPipelineCache[hash] = pipeline;

		VT_ENSURE(pipeline->IsValid());
		return pipeline;
	}

	RefPtr<RHI::ComputePipeline> PipelineStateCache::GetComputePipeline(RefPtr<RHI::Shader> computeShader)
	{
		VT_PROFILE_FUNCTION();

		std::scoped_lock lock{ s_instance->m_computePipelineCacheMutex };
		const size_t hash = Utility::GetComputeShaderHash(computeShader);

		if (s_instance->m_computePipelineCache.contains(hash))
		{
			auto pipeline = s_instance->m_computePipelineCache.at(hash);
			VT_ENSURE(pipeline->IsValid());

			return pipeline;
		}

		RefPtr<RHI::ComputePipeline> pipeline = RHI::ComputePipeline::Create(computeShader);
		s_instance->m_computePipelineCache[hash] = pipeline;

		VT_ENSURE(pipeline->IsValid());
		return pipeline;
	}
}
