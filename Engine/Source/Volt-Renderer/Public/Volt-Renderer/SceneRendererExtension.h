#pragma once

#include <RenderCore/RenderGraph/Resources/RenderGraphResourceHandle.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	class RenderScene;
	class Camera;

	enum class SceneRendererExtensionStage : uint32_t
	{
		PreGBuffer = 0,
		PostPostProcessing = 1
	};

	class SceneRendererExtension
	{
	public:
		SceneRendererExtension(Ref<RenderScene> renderScene)
			: m_renderScene(renderScene)
		{ }

		virtual ~SceneRendererExtension() = default;
		virtual RenderGraphImageHandle OnRender(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<Volt::Camera> camera, RenderGraphImageHandle prevOutputImage) = 0;

	protected:
		Weak<RenderScene> m_renderScene;
	};
}
