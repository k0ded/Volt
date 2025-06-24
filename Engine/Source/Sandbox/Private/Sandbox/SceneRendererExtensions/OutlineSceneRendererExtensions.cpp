#include "sbpch.h"

#include "Sandbox/SceneRendererExtensions/OutlineSceneRendererExtension.h"
#include "Sandbox/SceneRendererExtensions/OutlineTechnique.h"

#include <Volt-Renderer/RenderScene.h>
#include <Volt-Renderer/Utility/ScatteredBufferUpload.h>

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/Shader/ShaderMap.h>

using namespace Volt;

OutlineSceneRendererExtension::OutlineSceneRendererExtension(Ref<Volt::RenderScene> renderScene)
	: Volt::SceneRendererExtension(renderScene)
{
}

Volt::RGTextureRef OutlineSceneRendererExtension::OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage)
{
	if (m_selectedEntityIds.empty())
	{
		return prevOutputImage;
	}

	if (m_isSelectionDirty)
	{
		m_selectedPrimitivesSet.clear();

		for (const auto& entityId : m_selectedEntityIds)
		{
			m_selectedPrimitivesSet.insert(entityId);
		}
	}

	auto filterFunc = [this](const RenderPrimitiveData& primitiveData)
	{
		return m_selectedPrimitivesSet.contains(primitiveData.entityId);
	};

	OutlineTechnique outlineTechnique{ renderGraph, blackboard };
	outlineTechnique.Execute(prevOutputImage, *m_renderScene, view, filterFunc);

	return prevOutputImage;
}

void OutlineSceneRendererExtension::UpdateSelection(const Vector<EntityID>& entityIds)
{
	m_selectedEntityIds = entityIds;
	m_isSelectionDirty = true;
}
