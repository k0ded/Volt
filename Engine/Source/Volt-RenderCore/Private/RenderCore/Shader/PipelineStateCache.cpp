#include "rcpch.h"

#include "RenderCore/Shader/PipelineStateCache.h"

#include <CoreUtilities/ConsoleVariableRegistry.h>

#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	static ConsoleVariable<int32_t> g_pipelineStateCacheMaxPipelines(
		"r.PipelineStateCache.MaxPipelines",
		4096,
		"The maximum number of pipelines allowed in the cache.");

	namespace Utility
	{
		inline static const size_t GetComputeShaderHash(IntRef<RHI::Shader> shader)
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

			hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.topology));
			hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.cullMode));
			hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.fillMode));
			hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.depthMode));
			hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.depthCompareOperator));
			hash = Math::HashCombine(hash, std::hash<bool>()(pipelineInfo.enablePrimitiveRestart));

			hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.depthAttachmentFormat));

			for (size_t i = 0; i < RHI::MAX_COLOR_ATTACHMENT_COUNT; ++i)
			{
				if (i < pipelineInfo.colorAttachmentFormats.size())
				{
					hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.colorAttachmentFormats[i]));
				}

				hash = Math::HashCombine(hash, std::hash<bool>()(pipelineInfo.attachmentBlendStates[i].enabled));
				if (pipelineInfo.attachmentBlendStates[i].enabled)
				{
					hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.attachmentBlendStates[i].srcColorBlend));
					hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.attachmentBlendStates[i].dstColorBlend));
					hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.attachmentBlendStates[i].colorBlendOp));
					hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.attachmentBlendStates[i].srcAlphaBlend));
					hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.attachmentBlendStates[i].dstAlphaBlend));
					hash = Math::HashCombine(hash, Math::HashEnum(pipelineInfo.attachmentBlendStates[i].alphaBlendOp));
				}
			}


			return hash;
		}
	}

	PipelineStateCache::PipelineStateCache()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;

		m_renderPipelineCache.Resize(g_pipelineStateCacheMaxPipelines.GetValue());
		m_computePipelineCache.Resize(g_pipelineStateCacheMaxPipelines.GetValue());
	}
	
	PipelineStateCache::~PipelineStateCache()
	{
		s_instance = nullptr;
	}

	IntRef<RHI::RenderPipeline> PipelineStateCache::GetRenderPipeline(const RHI::RenderPipelineCreateInfo& pipelineInfo)
	{
		VT_PROFILE_FUNCTION();

		const size_t hash = Utility::GetRenderPipelineHash(pipelineInfo);

		auto& cacheEntry = s_instance->m_renderPipelineCache.Get(hash);

		PipelineCreationState expected = PipelineCreationState::Invalid;
		if (cacheEntry.state.compare_exchange_strong(expected, PipelineCreationState::Creating, std::memory_order::acq_rel))
		{
			// We should create the pipeline and fill the cache.
			cacheEntry.pipeline = RHI::RenderPipeline::Create(pipelineInfo);
			cacheEntry.state.store(PipelineCreationState::Created);

			return cacheEntry.pipeline;
		}
		else
		{
			if (expected == PipelineCreationState::Created)
			{
				return cacheEntry.pipeline;
			}
			else if (expected == PipelineCreationState::Creating)
			{
				// The cache is being created, let's create a temporary pipeline instead of waiting.
				return RHI::RenderPipeline::Create(pipelineInfo);
			}
		}

		VT_ENSURE_NO_ENTRY();
		return nullptr;
	}

	IntRef<RHI::ComputePipeline> PipelineStateCache::GetComputePipeline(IntRef<RHI::Shader> computeShader)
	{
		VT_PROFILE_FUNCTION();

		const size_t hash = Utility::GetComputeShaderHash(computeShader);

		auto& cacheEntry = s_instance->m_computePipelineCache.Get(hash);

		PipelineCreationState expected = PipelineCreationState::Invalid;
		if (cacheEntry.state.compare_exchange_strong(expected, PipelineCreationState::Creating, std::memory_order::acq_rel))
		{
			// We should create the pipeline and fill the cache.
			cacheEntry.pipeline = RHI::ComputePipeline::Create(computeShader);
			cacheEntry.state.store(PipelineCreationState::Created);

			return cacheEntry.pipeline;
		}
		else
		{
			if (expected == PipelineCreationState::Created)
			{
				return cacheEntry.pipeline;
			}
			else if (expected == PipelineCreationState::Creating)
			{
				// The cache is being created, let's create a temporary pipeline instead of waiting.
				return RHI::ComputePipeline::Create(computeShader);
			}
		}

		VT_ENSURE_NO_ENTRY();
		return nullptr;
	}

	void PipelineStateCache::InvalidatePipelinesWithReferenceToShader(IntRef<RHI::Shader> shader)
	{
		if (shader->GetShaderStage() == RHI::ShaderStage::Compute)
		{
			for (auto it = s_instance->m_computePipelineCache.GetIterator(); it; ++it)
			{
				if (it->pipeline)
				{
					if (it->pipeline->GetShader() == shader)
					{
						it->pipeline->Invalidate();
					}
				}
			}
		}
		else
		{
			for (auto it = s_instance->m_renderPipelineCache.GetIterator(); it; ++it)
			{
				if (it->pipeline)
				{
					for (const auto& pipelineShader : it->pipeline->GetShaders())
					{
						if (pipelineShader == shader)
						{
							it->pipeline->Invalidate();
							break;
						}
					}
				}
			}
		}
	}
}
