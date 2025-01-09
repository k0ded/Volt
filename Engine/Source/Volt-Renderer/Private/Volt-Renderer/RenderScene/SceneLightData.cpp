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
		DestroyPrimitive();
	}

	void SceneLightData::InitializeFromDescription(const SceneLightDescription& description)
	{
		m_description = description;

		if (m_renderSceneId != 0)
		{
			DestroyPrimitive();
		}

		m_renderSceneId = m_renderScene->AddLightInstance(m_relatedEntity, description);
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
