#pragma once

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	struct RenderView;

	class LightTileBinningTechnique
	{
	public:
		LightTileBinningTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		void Execute(const RenderView& view);

		inline static constexpr uint32_t TILE_SIZE = 16;

	private:
		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
