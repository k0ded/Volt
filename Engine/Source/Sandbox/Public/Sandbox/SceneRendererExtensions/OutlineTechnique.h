#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>
#include <Volt-Renderer/Mesh/MeshRenderer.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	class RenderScene;
	struct DrawCullingData;
	struct RenderView;
}

struct OutlineTechnique
{
	OutlineTechnique(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard);

	void Execute(Volt::RGTextureRef dstImage, Volt::RenderScene& renderScene, const Volt::RenderView& view, const Volt::MeshRenderer::PrimitveFilterFunc& primitiveFilter);

private:
	Volt::RGTextureRef AddDrawOutlineGeometryPass(Volt::RenderScene& renderScene, const Volt::RenderView& view, const Volt::MeshRenderer::PrimitveFilterFunc& primitiveFilter);
	Volt::RGTextureRef AddJumpFloodInitPass(Volt::RGTextureRef outlineGeometryImage, const Volt::RenderView& view);
	Volt::RGTextureRef AddJumpFloodPass(Volt::RGTextureRef prevImage, const Volt::RenderView& view, int32_t step);
	void AddOutlineCompositePass(Volt::RGTextureRef dstImage, const Volt::RenderView& view, Volt::RGTextureRef jumpfloodOutput);

	Volt::RenderGraph& m_renderGraph;
	Volt::RenderGraphBlackboard& m_blackboard;
};
