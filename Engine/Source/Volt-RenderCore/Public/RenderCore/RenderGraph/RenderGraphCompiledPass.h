#pragma once

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

		class PassBarriers
		{
		public:
			VT_NODISCARD VT_INLINE ArrayView<BarrierInfo> GetBarriers() const { return m_barriers; }
			VT_NODISCARD VT_INLINE Vector<BarrierInfo>& GetBarriersMutable() { return m_barriers; }
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
			Vector<BarrierInfo> m_barriers;
		};

		VT_INLINE void SetName(const std::string& name) { m_name = name; }
		VT_INLINE void AddSurrenderableResource(RGResource* resource) { m_surrenderableResources.emplace_back(resource); }
		VT_NODISCARD VT_INLINE const Vector<RGResource*>& GetSurrenderableResources() const { return m_surrenderableResources; }

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

		PassBarriers prePassBarriers;
		PassBarriers postPassBarriers;

	private:
		int32_t m_globalBarrierIndex = -1;
		int32_t m_postPassGlobalBarrierIndex = -1;

		Vector<RGResource*> m_surrenderableResources;
		std::string_view m_name;
	};
}
