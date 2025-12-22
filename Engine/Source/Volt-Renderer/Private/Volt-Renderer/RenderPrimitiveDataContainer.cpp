#include "vrpch.h"

#include "Volt-Renderer/RenderPrimitiveDataContainer.h"

namespace Volt
{
	RenderPrimitiveData* RenderPrimitiveDataContainer::Create(RenderPrimitiveID renderPrimitiveId)
	{
		RenderPrimitiveData* newRenderPrimitiveData = m_allocator.Allocate();
		newRenderPrimitiveData->id = renderPrimitiveId;
		m_renderPrimitiveDatas[renderPrimitiveId] = newRenderPrimitiveData;

		return newRenderPrimitiveData;
	}

	void RenderPrimitiveDataContainer::Destroy(RenderPrimitiveData* renderPrimitive)
	{
		VT_ENSURE(m_renderPrimitiveDatas.contains(renderPrimitive->id));
		m_renderPrimitiveDatas.erase(renderPrimitive->id);

		m_allocator.Free(renderPrimitive);
	}

	RenderPrimitiveData* RenderPrimitiveDataContainer::GetFromID(RenderPrimitiveID renderPrimitiveId) const
	{
		VT_ENSURE(m_renderPrimitiveDatas.contains(renderPrimitiveId));
		return m_renderPrimitiveDatas.at(renderPrimitiveId);
	}

	Vector<RenderPrimitiveData*> RenderPrimitiveDataContainer::GetRenderPrimitives() const
	{
		Vector<RenderPrimitiveData*> result;
		result.reserve(m_renderPrimitiveDatas.size());

		for (auto& [id, renderPrimitiveData] : m_renderPrimitiveDatas)
		{
			result.emplace_back(renderPrimitiveData);
		}

		return result;
	}
}
