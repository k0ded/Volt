#pragma once

#include <RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	class AutoExposureTechnique
	{
	public:
		AutoExposureTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

		void Execute(RenderGraphImageHandle srcRenderTarget, RenderGraphImageHandle averageLuminanceTarget, float deltaTime);

	private:
		RenderGraphBufferHandle GenerateLuminanceHistogram(RenderGraphImageHandle srcRenderTarget);
		void GenerateAverageLuminance(RenderGraphBufferHandle histogramBuffer, RenderGraphImageHandle averageLuminanceTarget, float deltaTime);

		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
