#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	struct RenderView;

	class BloomTechnique
	{
	public:
		inline static constexpr uint32_t NumMips = 5;

		BloomTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void Execute(const RenderView& view);

	private:
		RGTextureRef AddDownsamplePass(const RenderView& view);
		void AddUpsamplePass(const RenderView& view, RGTextureRef intermediateTexture);

		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
