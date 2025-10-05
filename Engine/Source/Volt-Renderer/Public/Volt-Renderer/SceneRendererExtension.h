#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	class RenderScene;
	class MeshPassProcessorRegistry;

	struct RenderView;

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
		virtual RGTextureRef OnRender(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, const RenderView& view, RGTextureRef prevOutputImage) = 0;
		virtual void OnRegistered(MeshPassProcessorRegistry& meshPassProcessorRegistry) {}

	protected:
		Weak<RenderScene> m_renderScene;
	};
}
