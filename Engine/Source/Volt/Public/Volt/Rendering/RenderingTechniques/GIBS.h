#pragma once

#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	namespace RHI
	{
		class StorageBuffer;
	}

	class GIBS
	{
	public:
		void Render(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

		bool m_allocate = true;

	private:
		void AllocateBuffers();

		RefPtr<RHI::StorageBuffer> m_surfelsBuffer;
		RefPtr<RHI::StorageBuffer> m_surfelsAllocatorBuffer;
		RefPtr<RHI::StorageBuffer> m_surfelGridBuffer;

	};
}
