#pragma once

#include <Volt-Renderer/SceneRendererExtension.h>

namespace Volt
{
	class DebugRenderer;
}

class DebugSceneRendererExtension : public Volt::SceneRendererExtension
{
public:
	DebugSceneRendererExtension(Ref<Volt::RenderScene> renderScene, Volt::DebugRenderer& debugRenderer);
	Volt::RGTextureRef OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage);

	VT_INLINE VT_NODISCARD RefPtr<Volt::RHI::Image> GetVisProxyIDImage() const { return m_visProxyIdImage; }

private:
	void RenderForwardLitDebugMeshes(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage);
	void RenderTranslucentDebugMeshes(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage);

	RefPtr<Volt::RHI::Image> m_visProxyIdImage;
	Volt::RGTextureRef m_rgVisProxyTexture;

	Volt::DebugRenderer& m_debugRenderer;
};
