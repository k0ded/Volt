#include "rhipch.h"

#include "RHIModule/Core/ResourceStateTracker.h"
#include "RHIModule/Images/Image.h"

namespace Volt::RHI
{
	void ResourceStateTracker::Initialize(RHIResource* resource, BarrierStage stage, BarrierAccess access, ImageLayout layout)
	{
		ResourceType resourceType = resource->GetType();

		if (resourceType == ResourceType::Image1D ||
			resourceType == ResourceType::Image2D ||
			resourceType == ResourceType::Image3D)
		{
			Image& image = resource->AsRef<Image>();

			const uint32_t numSubResources = image.GetDesc().mips * image.GetDesc().layers;
			m_subResourceStates.resize(numSubResources);

			for (uint32_t i = 0; i < numSubResources; ++i)
			{
				m_subResourceStates[i].stage = stage;
				m_subResourceStates[i].access = access;
				m_subResourceStates[i].layout = layout;
			}
		}
		else
		{
			m_subResourceStates.emplace_back(stage, access, layout);
		}
	}

	void ResourceStateTracker::Transition(uint32_t subResourceIndex, BarrierStage stage, BarrierAccess access, ImageLayout layout /*= ImageLayout::Undefined*/)
	{
		VT_ENSURE_MSG(!m_subResourceStates.empty(), "The resource state tracker has not been initialized!");
		VT_ENSURE(subResourceIndex < m_subResourceStates.size());

		std::scoped_lock lock{ m_resourceTrackerMutex };
		VT_PROFILE_LOCK_MARK(m_resourceTrackerMutex);

		ResourceState& state = m_subResourceStates[subResourceIndex];
		state.stage = stage;
		state.access = access;
		state.layout = layout;
	}
}
