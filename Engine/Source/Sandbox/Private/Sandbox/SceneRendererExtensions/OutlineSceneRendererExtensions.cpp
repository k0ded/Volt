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

#if 0
	if (m_selectedEntityIds.empty())
	{
		return prevOutputImage;
	}

	if (m_isSelectionDirty)
	{
		PagedVector<uint32_t> selectedPrimitives;

		for (const auto& entityId : m_selectedEntityIds)
		{
			selectedPrimitives.append(m_renderScene->GetPrimitiveIndicesFromEntityID(entityId));
		}

		const uint32_t numPrimitiveEntries = static_cast<uint32_t>(m_renderScene->GetMaxPrimitiveIndex()) / 32u;
		m_selectedPrimitivesMaskBuffer->GrowIfRequired(numPrimitiveEntries);

		RenderGraphBufferHandle selectedPrimitivesMask = renderGraph.AddExternalBuffer(m_selectedPrimitivesMaskBuffer->GetResource());

		// Clear to zero to make sure that no previously selected primitives are shown as selected.
		RGUtils::ClearBuffer(renderGraph, selectedPrimitivesMask, 0u);

		if (!selectedPrimitives.empty())
		{
			std::sort(selectedPrimitives.begin(), selectedPrimitives.end());

			uint32_t numValuesToUpdate = 0;

			for (int32_t prevMaskIndex = -1; const uint32_t primitiveIndex : selectedPrimitives)
			{
				const int32_t maskIndex = static_cast<int32_t>(primitiveIndex / 32u);
				if (maskIndex != prevMaskIndex)
				{
					numValuesToUpdate++;
					prevMaskIndex = maskIndex;
				}
			}

			ScatteredBufferUpload<uint32_t> bufferUpload{ numValuesToUpdate };

			uint32_t* currentMask = nullptr;
			for (int32_t prevMaskIndex = -1; const uint32_t primitiveIndex : selectedPrimitives)
			{
				const int32_t maskIndex = static_cast<int32_t>(primitiveIndex / 32);
				const uint32_t bitIndex = primitiveIndex % 32;
				if (maskIndex != prevMaskIndex)
				{
					currentMask = &bufferUpload.AddUploadItem(maskIndex);
					prevMaskIndex = maskIndex;
				}

				if (currentMask)
				{
					(*currentMask) |= (1u << bitIndex);
				}
			}

			bufferUpload.UploadTo(renderGraph, m_selectedPrimitivesMaskBuffer->GetResource());
		}

		m_isSelectionDirty = false;
	}

	OutlineTechnique outlineTechnique{ renderGraph, blackboard };
	outlineTechnique.Execute(renderGraph.AddExternalBuffer(m_selectedPrimitivesMaskBuffer->GetResource()), prevOutputImage, *m_renderScene);
#endif
	return prevOutputImage;
}

void OutlineSceneRendererExtension::UpdateSelection(const Vector<EntityID>& entityIds)
{
	m_selectedEntityIds = entityIds;
	m_isSelectionDirty = true;
}
