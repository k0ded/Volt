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

private:
	Volt::DebugRenderer& m_debugRenderer;
};
