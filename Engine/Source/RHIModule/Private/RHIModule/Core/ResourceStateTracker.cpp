#include "rhipch.h"
#include "RHIModule/Core/ResourceStateTracker.h"

#include "RHIModule/RHIModule.h"
#include "RHIModule/Images/Image.h"

namespace Volt::RHI
{
	void ResourceStateTracker::AddResource(RawPtr<RHIResource> resource, BarrierStage initialStage, BarrierAccess initialAccess, ImageLayout initialLayout)
	{
		std::scoped_lock lock{ m_mutex };
		VT_ENSURE(!m_resourceStates.contains(resource));

		ResourceType resourceType = resource->GetType();
		SubResourceStates& subResourceStates = m_resourceStates[resource];
		
		if (resourceType == ResourceType::Image1D ||
			resourceType == ResourceType::Image2D ||
			resourceType == ResourceType::Image3D)
		{
			Image& image = resource->AsRef<Image>();

			const uint32_t numSubResources = image.GetDesc().mips * image.GetDesc().layers;
			subResourceStates.resize(numSubResources);
		
			for (uint32_t i = 0; i < numSubResources; ++i)
			{
				subResourceStates[i].stage = initialStage;
				subResourceStates[i].access = initialAccess;
				subResourceStates[i].layout = initialLayout;
			}
		}
		else
		{
			subResourceStates.emplace_back(initialStage, initialAccess, initialLayout);
		}
	}
	
	void ResourceStateTracker::RemoveResource(RawPtr<RHIResource> resource)
	{
		std::scoped_lock lock{ m_mutex };
		VT_ENSURE(m_resourceStates.contains(resource));
		m_resourceStates.erase(resource);
	}
	
	void ResourceStateTracker::TransitionResource(RawPtr<RHIResource> resource, uint32_t subResourceIndex, BarrierStage dstStage, BarrierAccess dstAccess, ImageLayout dstLayout)
	{
		std::scoped_lock lock{ m_mutex };
		VT_ENSURE(m_resourceStates.contains(resource));
		VT_ENSURE(subResourceIndex < m_resourceStates.at(resource).size());

		ResourceState& state = m_resourceStates.at(resource)[subResourceIndex];
		state.stage = dstStage;
		state.access = dstAccess;
		state.layout = dstLayout;
	}
	
	const ResourceState& ResourceStateTracker::GetCurrentResourceState(RawPtr<RHIResource> resource, uint32_t subResourceIndex)
	{
		std::scoped_lock lock{ m_mutex };
		VT_ENSURE(m_resourceStates.contains(resource));

		return m_resourceStates.at(resource)[subResourceIndex];
	}
	
	void* ResourceStateTracker::GetHandleImpl() const
	{
		return nullptr;
	}
}
