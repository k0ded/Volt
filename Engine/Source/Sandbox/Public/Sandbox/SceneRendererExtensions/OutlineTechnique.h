#pragma once

#if 0
#include <RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	class RenderScene;
	struct DrawCullingData;
}

struct OutlineTechnique
{
	OutlineTechnique(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard);

	void Execute(Volt::RenderGraphBufferHandle selectedPrimitivesMask, Volt::RenderGraphImageHandle dstImage, Volt::RenderScene& renderScene);

private:
	Volt::RenderGraphImageHandle AddDrawOutlineGeometryPass(const Volt::DrawCullingData& cullingData);
	Volt::RenderGraphImageHandle AddJumpFloodInitPass(Volt::RenderGraphImageHandle outlineGeometryImage);
	Volt::RenderGraphImageHandle AddJumpFloodPass(Volt::RenderGraphImageHandle prevImage, int32_t step);
	void AddOutlineCompositePass(Volt::RenderGraphImageHandle dstImage, Volt::RenderGraphImageHandle jumpfloodOutput);

	Volt::RenderGraph& m_renderGraph;
	Volt::RenderGraphBlackboard& m_blackboard;
};
#endif
