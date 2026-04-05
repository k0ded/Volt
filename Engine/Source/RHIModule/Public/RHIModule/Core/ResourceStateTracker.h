#pragma once

#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Core/Core.h"

#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Locks/SpinMutex.h>
#include <CoreUtilities/Locks/ScopedLock.h>

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

		VT_INLINE ResourceState GetResourceState(uint32_t subResourceIndex) const
		{ 
			ScopedLock lock{ m_resourceTrackerMutex };
			VT_PROFILE_LOCK_MARK(m_resourceTrackerMutex);

			VT_ENSURE_MSG(!m_subResourceStates.empty(), "The resource state tracker has not been initialized!"); 
			VT_ENSURE(subResourceIndex < m_subResourceStates.size());
			return m_subResourceStates[subResourceIndex]; 
		}

	private:
		SubResourceStates m_subResourceStates;
		mutable VT_PROFILE_DECLARE_MUTEX(SpinMutex, m_resourceTrackerMutex);
	};
}
