#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraphGlobalAllocator.h"

namespace Volt
{
	RenderGraphDataAllocator* RenderGraphGlobalAllocator::GetAllocator()
	{
		return m_allocators.Allocate();
	}

	void RenderGraphGlobalAllocator::ReleaseAllocator(RenderGraphDataAllocator* allocator)
	{
		return m_allocators.Free(allocator);
	}

	RenderGraphGlobalAllocator& RenderGraphGlobalAllocator::Get()
	{
		static RenderGraphGlobalAllocator instance;
		return instance;
	}
}
