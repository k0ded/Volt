#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Pipelines/ComputePipeline.h>

#include <CoreUtilities/Containers/AtomicHashTable.h>

namespace Volt
{
	class VTRC_API PipelineStateCache
	{
	public:
		PipelineStateCache();
		~PipelineStateCache();

		static IntRef<RHI::RenderPipeline> GetRenderPipeline(const RHI::RenderPipelineCreateInfo& pipelineInfo);
		static IntRef<RHI::ComputePipeline> GetComputePipeline(IntRef<RHI::Shader> computeShader);
		static void InvalidatePipelinesWithReferenceToShader(IntRef<RHI::Shader> shader);

	private:
		enum class PipelineCreationState : uint8_t
		{
			Invalid,
			Creating,
			Created
		};

		template<typename T>
		class PipelineCache
		{
		public:
			struct Entry
			{
				Entry() = default;
				Entry(const Entry& other)
					: pipeline(other.pipeline),
					state(other.state.load())
				{}

				Entry& operator=(const Entry& other)
				{
					if (this != &other)
					{
						pipeline = other.pipeline;
						state = other.state.load();
					}
					return *this;
				}

				IntRef<T> pipeline;
				std::atomic<PipelineCreationState> state = PipelineCreationState::Invalid;
			};

			void Resize(uint32_t size)
			{
				m_hashTable.Reserve(size);
				m_cache.resize(size);
			}

			Entry& Get(size_t hash)
			{
				uint64_t outValue;
				VT_CHECK(m_hashTable.Insert(hash, outValue));

				return m_cache.at(outValue);
			}

			VT_INLINE ArrayView<Entry> GetCache() const { return m_cache; }

		private:
			AtomicHashTable<> m_hashTable;
			Vector<Entry> m_cache;
		};
		
		inline static PipelineStateCache* s_instance = nullptr;

		PipelineCache<RHI::RenderPipeline> m_renderPipelineCache;
		PipelineCache<RHI::ComputePipeline> m_computePipelineCache;
	};
}
