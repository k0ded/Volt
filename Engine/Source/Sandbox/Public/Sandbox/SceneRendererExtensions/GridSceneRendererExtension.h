#pragma once

#include <Volt-Renderer/SceneRendererExtension.h>

class GridSceneRendererExtension : public Volt::SceneRendererExtension
{
public:
	GridSceneRendererExtension(Volt::RenderScene* renderScene)
		: Volt::SceneRendererExtension(renderScene)
	{ }

	~GridSceneRendererExtension() override = default;
	Volt::RGTextureRef OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage) override;
};
