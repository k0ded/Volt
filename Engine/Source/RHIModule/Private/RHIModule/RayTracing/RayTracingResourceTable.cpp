#include "rhipch.h"

#include "RHIModule/RayTracing/RayTracingResuorceTable.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<RayTracingResourceTable> RayTracingResourceTable::Create()
	{
		return RHIModule::GetInstance().CreateRayTracingResourceTable();
	}

	RayTracingResourceTable::ResourceIndices::ResourceIndices()
		: m_nextIndex(0)
	{
		m_availableIndices.Allocate(RayTracingResourceTable::MaxSize);
	}

	uint32_t RayTracingResourceTable::ResourceIndices::Allocate()
	{
		uint32_t result;
		if (!m_availableIndices.Pop(result))
		{
			result = m_nextIndex.fetch_add(1u, std::memory_order::relaxed);
		}
		VT_ENSURE(result < RayTracingResourceTable::MaxSize);
		return result;
	}

	void RayTracingResourceTable::ResourceIndices::Free(uint32_t index)
	{
		m_availableIndices.Push(index);
	}
}
