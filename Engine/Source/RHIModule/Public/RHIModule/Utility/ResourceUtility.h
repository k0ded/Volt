#pragma once

#include "RHIModule/Core/RHICommon.h"

#include "RHIModule/Graphics/GraphicsContext.h"

namespace Volt::RHI::ResourceUtility
{
	inline void InitializeBarrierSrcFromCurrentState(ImageBarrier& barrier, RawPtr<RHIResource> resource)
	{
		const ResourceState& currentState = resource->GetResourceStateTracker().GetResourceState(0);
		barrier.srcStage = currentState.stage;
		barrier.srcAccess = currentState.access;
		barrier.srcLayout = currentState.layout;
	}

	inline void InitializeBarrierSrcFromCurrentState(BufferBarrier& barrier, RawPtr<RHIResource> resource)
	{
		const ResourceState& currentState = resource->GetResourceStateTracker().GetResourceState(0);
		barrier.srcAccess = currentState.access;
		barrier.srcStage = currentState.stage;
	}
}
