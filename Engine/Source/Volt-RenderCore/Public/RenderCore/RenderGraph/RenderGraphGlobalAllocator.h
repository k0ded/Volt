#pragma once

#include "RenderCore/RenderGraph/RenderGraphDataAllocator.h"

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

namespace Volt
{
	class RenderGraphGlobalAllocator
	{
	public:
		RenderGraphDataAllocator* GetAllocator();
		void ReleaseAllocator(RenderGraphDataAllocator* allocator);

		static RenderGraphGlobalAllocator& Get();

	private:
		PagedAtomicArenaAllocator<RenderGraphDataAllocator, 32> m_allocators;
	};
}
