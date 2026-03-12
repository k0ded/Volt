#pragma once

#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Core/Core.h"

#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/Profiling/Profiling.h>

#include <shared_mutex>

namespace Volt::RHI
{
	struct ResourceState
	{
		BarrierStage stage;
		BarrierAccess access;
		ImageLayout layout;
	};

	class ResourceStateTracker
	{
	public:
		using SubResourceStates = InlineVector<ResourceState, 1>;

		VTRHI_API void Initialize(RHIResource* resource, BarrierStage stage, BarrierAccess access, ImageLayout layout = ImageLayout::Undefined);
		VTRHI_API void Transition(uint32_t subResourceIndex, BarrierStage stage, BarrierAccess access, ImageLayout layout = ImageLayout::Undefined);

		VT_INLINE const ResourceState& GetResourceState(uint32_t subResourceIndex) const
		{ 
			std::shared_lock lock{ m_resourceTrackerMutex };
			VT_PROFILE_LOCK_MARK(m_resourceTrackerMutex);

			VT_ENSURE_MSG(!m_subResourceStates.empty(), "The resource state tracker has not been initialized!"); 
			return m_subResourceStates[subResourceIndex]; 
		}

	private:
		SubResourceStates m_subResourceStates;
		mutable VT_PROFILE_DECLARE_MUTEX_SHARED(std::shared_mutex, m_resourceTrackerMutex);
	};
}
