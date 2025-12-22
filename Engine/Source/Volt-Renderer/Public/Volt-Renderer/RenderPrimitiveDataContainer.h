#pragma once

#include "Volt-Renderer/RenderPrimitiveData.h"

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>
#include <CoreUtilities/Containers/Map.h>

namespace Volt
{
	class RenderPrimitiveDataContainer
	{
	public:
		RenderPrimitiveData* Create(RenderPrimitiveID renderPrimitiveId);
		void Destroy(RenderPrimitiveData* renderPrimitive);
		RenderPrimitiveData* GetFromID(RenderPrimitiveID renderPrimitiveId) const;

		Vector<RenderPrimitiveData*> GetRenderPrimitives() const;

	private:
		PagedAtomicArenaAllocator<RenderPrimitiveData, 1024> m_allocator;
		Map<RenderPrimitiveID, RenderPrimitiveData*> m_renderPrimitiveDatas;
	};
}
