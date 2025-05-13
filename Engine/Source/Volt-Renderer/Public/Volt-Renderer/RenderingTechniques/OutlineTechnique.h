#pragma once

#include <RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	class RenderScene;
	struct DrawCullingData;

	struct OutlineTechnique
	{
		OutlineTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);

		void Execute(RenderGraphBufferHandle selectedPrimitivesMask, RenderGraphImageHandle dstImage, RenderScene& renderScene);

	private:
		RenderGraphImageHandle AddDrawOutlineGeometryPass(const DrawCullingData& cullingData);
		RenderGraphImageHandle AddJumpFloodInitPass(RenderGraphImageHandle outlineGeometryImage);
		RenderGraphImageHandle AddJumpFloodPass(RenderGraphImageHandle prevImage, int32_t step);
		void AddOutlineCompositePass(RenderGraphImageHandle dstImage, RenderGraphImageHandle jumpfloodOutput);

		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
