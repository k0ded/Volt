#include "rhipch.h"

#include "RHIModule/Descriptors/ResourceTable.h"
#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	RefPtr<ResourceTable> ResourceTable::Create()
	{
		return RHIModule::GetInstance().CreateResourceTable();
	}

	ResourceTable::ResourceIndices::ResourceIndices()
		: m_nextIndex(0)
	{
		m_availableIndices.Allocate(ResourceTable::MaxSize);
	}

	uint32_t ResourceTable::ResourceIndices::Allocate()
	{
		uint32_t result;
		if (!m_availableIndices.Pop(result))
		{
			result = m_nextIndex.fetch_add(1u, std::memory_order::relaxed);
		}
		VT_ENSURE(result < ResourceTable::MaxSize);
		return result;
	}

	void ResourceTable::ResourceIndices::Free(uint32_t index)
	{
		m_availableIndices.Push(index);
	}
}
