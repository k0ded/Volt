#pragma once

#include <Volt-Renderer/SceneRendererExtension.h>

class GridSceneRendererExtension : public Volt::SceneRendererExtension
{
public:
	GridSceneRendererExtension(Ref<Volt::RenderScene> renderScene)
		: Volt::SceneRendererExtension(renderScene)
	{ }

	~GridSceneRendererExtension() override = default;
	Volt::RGTextureRef OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, Ref<Volt::Camera> camera, Volt::RGTextureRef prevOutputImage) override;
};
