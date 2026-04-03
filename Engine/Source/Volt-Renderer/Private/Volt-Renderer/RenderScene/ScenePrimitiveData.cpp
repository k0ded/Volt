#include "vrpch.h"

#include "Volt-Renderer/RenderScene/ScenePrimitiveData.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RendererUtilities.h"
#include "Volt-Renderer/RayTracing/RayTracingScene.h"

#include <RHIModule/RHIFeatures.h>

VT_DEFINE_LOG_CATEGORY(LogScenePrimitiveData);

namespace Volt
{
	ScenePrimitiveData::ScenePrimitiveData(const EntityID& relatedEntity, RenderScene* renderScene)
		: m_relatedEntity(relatedEntity),
		m_renderScene(renderScene)
	{
	}

	ScenePrimitiveData::ScenePrimitiveData(const EntityID& relatedEntity, RenderScene* renderScene, Ref<TempAnimator> animator)
		: m_relatedEntity(relatedEntity),
		m_renderScene(renderScene),
		m_animator(animator)
	{

	}

	ScenePrimitiveData::~ScenePrimitiveData()
	{
		DestroyScenePrimitives();
	}

	void ScenePrimitiveData::InitializeFromDescription(const ScenePrimitiveDescription& description)
	{
		VT_ENSURE(m_renderScene);
		VT_ENSURE(description.primitiveMesh);
	
		if (!m_renderObjects.empty())
		{
			DestroyScenePrimitives();
		}

		for (uint32_t index = 0; const auto& renderMaterial : description.materials)
		{
			m_primitiveMaterialTable.SetMaterial(renderMaterial, index);
			index++;
		}

		m_primitiveMesh = description.primitiveMesh;
		CreateScenePrimitives();
	}

	void ScenePrimitiveData::Invalidate()
	{
		for (const auto& id : m_renderObjects)
		{
			m_renderScene->InvalidatePrimitiveInstance(id);
		}

		if (RHI::RHICanUseRayTracing())
		{
			m_renderScene->InvalidateRayTracingInstance(m_rayTracingInstance);
		}
	}

	void ScenePrimitiveData::CreateScenePrimitives()
	{
		VT_ENSURE(m_primitiveMesh);

		const auto& meshMaterialTable = m_primitiveMesh->GetMaterialTable();

		MaterialTable finalMaterialTable;
		for (uint32_t i = 0; i < meshMaterialTable.GetSize(); i++)
		{
			if (m_primitiveMaterialTable.ContainsMaterialIndex(i) && m_primitiveMaterialTable.GetMaterial(i))
			{
				finalMaterialTable.SetMaterial(m_primitiveMaterialTable.GetMaterial(i), i);
			}
			else
			{
				finalMaterialTable.SetMaterial(meshMaterialTable.GetMaterial(i), i);
			}
		}

		const auto& subMeshes = m_primitiveMesh->GetSubMeshes();
		for (size_t i = 0; i < subMeshes.size(); i++)
		{
			const uint32_t materialIndex = subMeshes.at(i).materialIndex;
			VT_ENSURE(finalMaterialTable.ContainsMaterialIndex(materialIndex));

			Ref<RenderMaterial> material = finalMaterialTable.GetMaterial(materialIndex);

			if (!material)
			{
				VT_LOGC(Warning, LogScenePrimitiveData, "Mesh {} has an invalid material at index {}! Assigning a default material.", m_primitiveMesh->GetName(), materialIndex);
				material = RendererUtilities::GetDefaultResources().defaultMaterial;
			}

			RenderPrimitiveID renderObjectId = m_renderScene->AddPrimitiveInstance(m_relatedEntity, m_animator, m_primitiveMesh, material, static_cast<uint32_t>(i));
			m_renderObjects.emplace_back(renderObjectId);
		}

		if (RHI::RHICanUseRayTracing())
		{
			m_rayTracingInstance = m_renderScene->AddRayTracingInstance(m_relatedEntity, m_primitiveMesh, m_renderObjects.front());
		}
	}

	void ScenePrimitiveData::DestroyScenePrimitives()
	{
		VT_ENSURE(m_renderScene);

		if (RHI::RHICanUseRayTracing())
		{
			m_renderScene->RemoveRayTracingInstance(m_rayTracingInstance);
		}

		for (const auto& id : m_renderObjects)
		{
			m_renderScene->RemovePrimitiveInstance(id);
		}

		m_renderObjects.clear();
	}
}
