#pragma once

#include <RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	class ScreenSpaceReflections
	{
	public:
		ScreenSpaceReflections(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

		void Execute(RenderGraphImageHandle sceneColor);

	private:
		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
