#pragma once

#include <Volt-Renderer/SceneRendererExtension.h>

#include <EntitySystem/EntityID.h>
#include <RenderCore/Resources/GrowingGPUBuffer.h>

#include <unordered_set>

class OutlineSceneRendererExtension : public Volt::SceneRendererExtension
{
public:
	OutlineSceneRendererExtension(Ref<Volt::RenderScene> renderScene);
	~OutlineSceneRendererExtension() override = default;

	Volt::RGTextureRef OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage) override;
	void OnRegistered(Volt::MeshPassProcessorRegistry& meshPassProcessorRegistry) override;

	void UpdateSelection(const Vector<Volt::EntityID>& entityIds);

private:
	Vector<Volt::EntityID> m_selectedEntityIds;
	std::unordered_set<Volt::EntityID> m_selectedPrimitivesSet;
	bool m_isSelectionDirty = false;

	class OutlinePassMeshProcessor* m_meshPassProcessor = nullptr;
};
