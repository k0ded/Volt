#pragma once

#include "Volt-Renderer/SceneRendererStructs.h"

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	class RenderScene;
	class Camera;

	struct DirectionalLightUniformBuffer;

	class DirectionalShadowTechnique
	{
	public:
		DirectionalShadowTechnique(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard);
		DirectionalShadowData Execute(Ref<Camera> camera, Ref<RenderScene> renderScene);

	private:
		RenderGraph& m_renderGraph;
		RenderGraphBlackboard& m_blackboard;
	};
}
