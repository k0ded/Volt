#pragma once

#include <Volt-Renderer/SceneRendererExtension.h>

namespace Volt
{
	class DebugRenderer;
}

class DebugSceneRendererExtension : public Volt::SceneRendererExtension
{
public:
	DebugSceneRendererExtension(Volt::RenderScene* renderScene, Volt::DebugRenderer& debugRenderer);
	Volt::RGTextureRef OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage);

	VT_INLINE VT_NODISCARD IntRef<Volt::RHI::Image> GetVisProxyIDImage() const { return m_visProxyIdImage; }

private:
	void RenderForwardLitDebugMeshes(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage);
	void RenderTranslucentDebugMeshes(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage);

	IntRef<Volt::RHI::Image> m_visProxyIdImage;

	Volt::RGTextureRef m_rgVisProxyTexture;
	Volt::RGTextureRef m_depthTexture;

	Volt::DebugRenderer& m_debugRenderer;
};
