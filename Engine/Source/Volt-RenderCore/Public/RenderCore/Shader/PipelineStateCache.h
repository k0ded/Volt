#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Pipelines/ComputePipeline.h>

#include <CoreUtilities/Containers/AtomicHashTable.h>
#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

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

			using EntryAllocator = PagedAtomicArenaAllocator<Entry, 128, DefaultHeapAllocator, true>;

			void Resize(uint32_t size)
			{
				m_hashTable.Reserve(size);
			}

			Entry& Get(size_t hash)
			{
				Optional<Entry*> value = m_hashTable.Find(hash);
				if (value.HasValue())
				{
					return *value.Get();
				}

				Entry* newEntry = m_entryAllocator.Allocate();
				value = m_hashTable.GetOrInsert(hash, newEntry);

				if (value.HasValue())
				{
					if (value.Get() != newEntry)
					{
						m_entryAllocator.Free(newEntry);
					}

					return *value.Get();
				}

				VT_ENSURE(false);
				return *value.Get();
			}
			
			VT_INLINE EntryAllocator::Iterator GetIterator() const { return EntryAllocator::Iterator(m_entryAllocator); }

		private:
			AtomicHashTable<Entry*> m_hashTable;
			EntryAllocator m_entryAllocator;
		};
		
		inline static PipelineStateCache* s_instance = nullptr;

		PipelineCache<RHI::RenderPipeline> m_renderPipelineCache;
		PipelineCache<RHI::ComputePipeline> m_computePipelineCache;
	};
}
