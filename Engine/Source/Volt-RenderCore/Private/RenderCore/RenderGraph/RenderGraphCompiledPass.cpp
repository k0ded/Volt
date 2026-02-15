#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraphCompiledPass.h"

namespace Volt
{

	RGCompiledPass::RGCompiledPass(RenderGraphDataAllocator* dataAllocator)
		: prePassBarriers(dataAllocator),
		postPassBarriers(dataAllocator)
	{}

	RGCompiledPass::PassBarriers::PassBarriers(RenderGraphDataAllocator* dataAllocator)
	{
		m_barriers.set_allocator({ dataAllocator });
	}
}
