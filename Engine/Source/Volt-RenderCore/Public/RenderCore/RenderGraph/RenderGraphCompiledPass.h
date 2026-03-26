#pragma once

#include "RenderCore/RenderGraph/RenderGraphContainerAllocator.h"

#include <RHIModule/Core/RHICommon.h>

#include <CoreUtilities/Containers/ArrayView.h>

namespace Volt
{
	class RGResource;

	class RGCompiledPass
	{
	public:
		struct BarrierInfo
		{
			RHI::ResourceBarrierInfo barrier;
			RGResource* resource = nullptr;
		};

		struct ResourceAllocationEvent
		{
			RGResource* resource;
			uint32_t lifetime;
		};

		class PassBarriers
		{
		public:
			PassBarriers(RenderGraphDataAllocator* dataAllocator);

			VT_NODISCARD VT_INLINE ArrayView<BarrierInfo> GetBarriers() const { return m_barriers; }
			VT_NODISCARD VT_INLINE RGVector<BarrierInfo>& GetBarriersMutable() { return m_barriers; }
			VT_NODISCARD VT_INLINE size_t GetBarrierCount() const { return m_barriers.size(); }
			VT_NODISCARD VT_INLINE bool Empty() const { return m_barriers.empty(); }

			VT_NODISCARD VT_INLINE RHI::ResourceBarrierInfo& AddBarrier(RHI::BarrierType type, RGResource* resource = nullptr)
			{
				auto& barrierInfo = m_barriers.emplace_back();
				if (type == RHI::BarrierType::Image)
				{
					barrierInfo.barrier = RHI::ResourceBarrierInfo::InitializeAsImageBarrier();
				}
				else if (type == RHI::BarrierType::Buffer)
				{
					barrierInfo.barrier = RHI::ResourceBarrierInfo::InitializeAsBufferBarrier();
				}
				else if (type == RHI::BarrierType::Global)
				{
					barrierInfo.barrier = RHI::ResourceBarrierInfo::InitializeAsGlobalBarrier();
				}

				barrierInfo.resource = resource;
				return barrierInfo.barrier;
			}

			VT_NODISCARD VT_INLINE RHI::ResourceBarrierInfo& GetBarrier(size_t index)
			{
				return m_barriers.at(index).barrier;
			}

		private:
			RGVector<BarrierInfo> m_barriers;
		};

		RGCompiledPass(RenderGraphDataAllocator* dataAllocator);

		VT_INLINE void SetName(const String& name) { m_name = name; }
		VT_INLINE void SetPassIndex(uint32_t passIndex) { m_passIndex = passIndex; }
		VT_INLINE uint32_t GetPassIndex() const { return m_passIndex; }

		VT_INLINE ArrayView<ResourceAllocationEvent> GetTransientResourceAllocations() const { return m_transientResourceAllocations; }
		VT_INLINE ArrayView<RGResource*> GetTransientResourceFrees() const { return m_transientResourceFrees; }

		// We only want maximum ONE global barrier per pass. As a single global barrier
		// can represent multiple.
		inline RHI::GlobalBarrier& GetGlobalBarrier()
		{
			if (m_globalBarrierIndex == -1)
			{
				m_globalBarrierIndex = static_cast<int32_t>(prePassBarriers.GetBarrierCount());
				auto& barrier = prePassBarriers.AddBarrier(RHI::BarrierType::Global);
				return barrier.globalBarrier();
			}

			return prePassBarriers.GetBarrier(static_cast<size_t>(m_globalBarrierIndex)).globalBarrier();
		}

		inline RHI::ResourceBarrierInfo& GetGlobalBarrierInfo()
		{
			if (m_globalBarrierIndex == -1)
			{
				m_globalBarrierIndex = static_cast<int32_t>(prePassBarriers.GetBarrierCount());
				auto& barrier = prePassBarriers.AddBarrier(RHI::BarrierType::Global);
				return barrier;
			}

			return prePassBarriers.GetBarrier(static_cast<size_t>(m_globalBarrierIndex));
		}

		inline RHI::GlobalBarrier& GetPostPassGlobalBarrier()
		{
			if (m_postPassGlobalBarrierIndex == -1)
			{
				m_postPassGlobalBarrierIndex = static_cast<int32_t>(postPassBarriers.GetBarrierCount());
				auto& barrier = postPassBarriers.AddBarrier(RHI::BarrierType::Global);
				return barrier.globalBarrier();
			}

			return postPassBarriers.GetBarrier(static_cast<size_t>(m_postPassGlobalBarrierIndex)).globalBarrier();
		}

		inline void AddTransientResourceAllocation(RGResource* resource, uint32_t resourceLifetime)
		{
			m_transientResourceAllocations.emplace_back(resource, resourceLifetime);
		}

		inline void AddTransientResourceFree(RGResource* resource)
		{
			m_transientResourceFrees.emplace_back(resource);
		}

		inline void SortTransientResourceAllocations()
		{
			std::sort(m_transientResourceAllocations.begin(), m_transientResourceAllocations.end(), [](const ResourceAllocationEvent& lhs, const ResourceAllocationEvent& rhs)
			{
				return lhs.lifetime > rhs.lifetime;
			});
		}

		PassBarriers prePassBarriers;
		PassBarriers postPassBarriers;

	private:
		int32_t m_globalBarrierIndex = -1;
		int32_t m_postPassGlobalBarrierIndex = -1;
		uint32_t m_passIndex = 0xFFFFFFFF;

		RGVector<ResourceAllocationEvent> m_transientResourceAllocations;
		RGVector<RGResource*> m_transientResourceFrees;

		StringView m_name;
	};
}
