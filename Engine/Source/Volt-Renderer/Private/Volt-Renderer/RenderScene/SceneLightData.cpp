#include "vrpch.h"

#include "Volt-Renderer/RenderScene/SceneLightData.h"
#include "Volt-Renderer/RenderScene.h"

namespace Volt
{
	SceneLightData::SceneLightData(const EntityID& entityId, RenderScene* renderScene)
		: m_relatedEntity(entityId),
		m_renderScene(renderScene)
	{
	}

	SceneLightData::~SceneLightData()
	{
		if (m_renderSceneId != 0)
		{
			DestroyPrimitive();
		}
	}

	void SceneLightData::InitializeFromDescription(const SceneLightDescription& description)
	{
		m_description = description;

		// Clamp values
		m_description.radius = glm::max(m_description.radius, 0.f);
		m_description.range = glm::max(m_description.range, 0.f);
		m_description.innerAngle = glm::max(m_description.innerAngle, 0.f);
		m_description.outerAngle = glm::max(m_description.outerAngle, 0.f);
		m_description.sunRadius = glm::max(m_description.sunRadius, 0.f);
		m_description.lod = glm::max(m_description.lod, 0.f);
		m_description.intensity = glm::max(m_description.intensity, 0.f);
		m_description.falloff = glm::max(m_description.falloff, 0.f);

		if (m_renderSceneId != 0)
		{
			DestroyPrimitive();
		}

		m_renderSceneId = m_renderScene->AddLightInstance(m_relatedEntity, m_description);
	}

	void SceneLightData::Invalidate()
	{
		m_renderScene->InvalidateLightInstance(m_renderSceneId);
	}

	void SceneLightData::DestroyPrimitive()
	{
		VT_ENSURE(m_renderScene);

		m_renderScene->RemoveLightInstance(m_renderSceneId);
		m_renderSceneId = 0;
	}
}
