#pragma once

#if 0
#include <Volt-Renderer/SceneRendererExtension.h>

#include <EntitySystem/EntityID.h>
#include <RenderCore/Resources/GrowingGPUBuffer.h>

class OutlineSceneRendererExtension : public Volt::SceneRendererExtension
{
public:
	OutlineSceneRendererExtension(Ref<Volt::RenderScene> renderScene);
	~OutlineSceneRendererExtension() override = default;

	Volt::RenderGraphImageHandle OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, Ref<Volt::Camera> camera, Volt::RenderGraphImageHandle prevOutputImage) override;

	void UpdateSelection(const Vector<Volt::EntityID>& entityIds);

private:
	Vector<Volt::EntityID> m_selectedEntityIds;
	Ref<Volt::GrowingGPUBuffer> m_selectedPrimitivesMaskBuffer;
	bool m_isSelectionDirty = false;
};
#endif
