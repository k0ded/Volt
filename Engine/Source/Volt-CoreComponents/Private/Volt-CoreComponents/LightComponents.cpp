#include "vtccpch.h"

#include "Volt-CoreComponents/LightComponents.h"

#include <Volt-Assets/StreamingManager.h>

#include <Volt-Renderer/Renderer.h>

namespace Volt
{
	namespace Utility
	{
		SceneLightDescription InitializeLightDescription(const PointLightComponent& component)
		{
			SceneLightDescription lightDescription;
			lightDescription.lightType = SceneLightType::Point;
			lightDescription.radius = component.radius;
			lightDescription.intensity = component.intensity;
			lightDescription.color = component.color;
			lightDescription.falloff = component.falloff;
			lightDescription.castShadows = component.castShadows;

			return lightDescription;
		}

		SceneLightDescription InitializeLightDescription(const SpotLightComponent& component, const glm::vec3& direction)
		{
			SceneLightDescription lightDescription;
			lightDescription.lightType = SceneLightType::Spot;
			lightDescription.range = component.range;
			lightDescription.innerAngle = component.innerAngle;
			lightDescription.outerAngle = component.outerAngle;
			lightDescription.intensity = component.intensity;
			lightDescription.color = component.color;
			lightDescription.falloff = component.falloff;
			lightDescription.castShadows = component.castShadows;
			lightDescription.direction = direction;

			return lightDescription;
		}

		SceneLightDescription InitializeLightDescription(const DirectionalLightComponent& component, const glm::vec3& direction)
		{
			SceneLightDescription lightDescription;
			lightDescription.lightType = SceneLightType::Directional;
			lightDescription.intensity = component.intensity;
			lightDescription.color = component.color;
			lightDescription.castShadows = component.castShadows;
			lightDescription.sunRadius = component.sunRadius;
			lightDescription.direction = direction;

			return lightDescription;
		}

		SceneLightDescription InitializeLightDescription(const SkylightComponent& component)
		{
			SceneLightDescription lightDescription;
			lightDescription.lightType = SceneLightType::Sky;
			lightDescription.intensity = component.intensity;
			lightDescription.lod = component.lod;
			lightDescription.show = component.show;

			return lightDescription;
		}

		StreamingInstanceDescription CreateStreamingInstanceDescription(const SkylightComponent& skylightComponent, EntityID entityId, Ref<SceneLightData> sceneLightData)
		{
			StreamingInstanceDescription streamingInstanceDesc;
			streamingInstanceDesc.entityId = entityId;
			streamingInstanceDesc.environmentTextureHandle = skylightComponent.environmentTextureHandle;
			streamingInstanceDesc.sceneLightData = sceneLightData;
			streamingInstanceDesc.sceneLightDescription = InitializeLightDescription(skylightComponent);

			return streamingInstanceDesc;
		}
	}

	void PointLightComponent::OnInitialize(LightEntity entity)
	{
		auto& component = entity.GetComponent<PointLightComponent>();
		component.m_sceneLightData = CreateRef<SceneLightData>(entity.GetID(), entity.GetRenderScene());
		component.m_sceneLightData->InitializeFromDescription(Utility::InitializeLightDescription(component));
	}

	void PointLightComponent::OnDestroy(LightEntity entity)
	{
		auto& component = entity.GetComponent<PointLightComponent>();
		component.m_sceneLightData = nullptr;
	}

	void PointLightComponent::OnTransformChanged(LightEntity entity)
	{
		auto& component = entity.GetComponent<PointLightComponent>();
		VT_ENSURE(component.m_sceneLightData);

		component.m_sceneLightData->Invalidate();
	}

	void PointLightComponent::OnComponentCopied(LightEntity entity)
	{
		auto& component = entity.GetComponent<PointLightComponent>();
		VT_ENSURE(component.m_sceneLightData);

		component.m_sceneLightData->InitializeFromDescription(Utility::InitializeLightDescription(component));
	}

	void PointLightComponent::OnMemberChanged(LightEntity entity)
	{
		auto& component = entity.GetComponent<PointLightComponent>();
		VT_ENSURE(component.m_sceneLightData);

		component.m_sceneLightData->InitializeFromDescription(Utility::InitializeLightDescription(component));
	}
	
	void SpotLightComponent::OnInitialize(LightEntity entity)
	{
		auto& component = entity.GetComponent<SpotLightComponent>();
		component.m_sceneLightData = CreateRef<SceneLightData>(entity.GetID(), entity.GetRenderScene());
		component.m_sceneLightData->InitializeFromDescription(Utility::InitializeLightDescription(component, entity.GetForward() * -1.f));
	}

	void SpotLightComponent::OnDestroy(LightEntity entity)
	{
		auto& component = entity.GetComponent<SpotLightComponent>();
		component.m_sceneLightData = nullptr;
	}

	void SpotLightComponent::OnTransformChanged(LightEntity entity)
	{
		auto& component = entity.GetComponent<SpotLightComponent>();
		VT_ENSURE(component.m_sceneLightData);

		component.m_sceneLightData->Invalidate();
	}

	void SpotLightComponent::OnComponentCopied(LightEntity entity)
	{
		auto& component = entity.GetComponent<SpotLightComponent>();
		VT_ENSURE(component.m_sceneLightData);

		component.m_sceneLightData->InitializeFromDescription(Utility::InitializeLightDescription(component, entity.GetForward() * -1.f));
	}

	void SpotLightComponent::OnMemberChanged(LightEntity entity)
	{
		auto& component = entity.GetComponent<SpotLightComponent>();
		VT_ENSURE(component.m_sceneLightData);

		component.m_sceneLightData->InitializeFromDescription(Utility::InitializeLightDescription(component, entity.GetForward() * -1.f));
	}

	void DirectionalLightComponent::OnInitialize(LightEntity entity)
	{
		auto& component = entity.GetComponent<DirectionalLightComponent>();
		component.m_sceneLightData = CreateRef<SceneLightData>(entity.GetID(), entity.GetRenderScene());
		component.m_sceneLightData->InitializeFromDescription(Utility::InitializeLightDescription(component, entity.GetForward() * -1.f));
	}

	void DirectionalLightComponent::OnDestroy(LightEntity entity)
	{
		auto& component = entity.GetComponent<DirectionalLightComponent>();
		component.m_sceneLightData = nullptr;
	}

	void DirectionalLightComponent::OnTransformChanged(LightEntity entity)
	{
		auto& component = entity.GetComponent<DirectionalLightComponent>();
		VT_ENSURE(component.m_sceneLightData);

		component.m_sceneLightData->Invalidate();
	}

	void DirectionalLightComponent::OnComponentCopied(LightEntity entity)
	{
		auto& component = entity.GetComponent<DirectionalLightComponent>();
		VT_ENSURE(component.m_sceneLightData);

		component.m_sceneLightData->InitializeFromDescription(Utility::InitializeLightDescription(component, entity.GetForward() * -1.f));
	}

	void DirectionalLightComponent::OnMemberChanged(LightEntity entity)
	{
		auto& component = entity.GetComponent<DirectionalLightComponent>();
		VT_ENSURE(component.m_sceneLightData);

		component.m_sceneLightData->InitializeFromDescription(Utility::InitializeLightDescription(component, entity.GetForward() * -1.f));
	}

	void SkylightComponent::UpdateSceneLightData(EntityID entityId)
	{
		VT_ENSURE(m_sceneLightData);

		if (environmentTextureHandle == Asset::Null())
		{
			return;
		}
		StreamingManager::Get().InvalidateInstance(m_streamingInstanceID, Utility::CreateStreamingInstanceDescription(*this, entityId, m_sceneLightData));
	}

	void SkylightComponent::OnInitialize(LightEntity entity)
	{
		auto& component = entity.GetComponent<SkylightComponent>();
		component.m_sceneLightData = CreateRef<SceneLightData>(entity.GetID(), entity.GetRenderScene());
		component.m_streamingInstanceID = StreamingManager::Get().AddInstance(Utility::CreateStreamingInstanceDescription(component, entity.GetID(), component.m_sceneLightData));
	}

	void SkylightComponent::OnDestroy(LightEntity entity)
	{
		auto& component = entity.GetComponent<SkylightComponent>();

		StreamingManager::Get().RemoveInstance(component.m_streamingInstanceID);
		component.m_sceneLightData = nullptr;
	}

	void SkylightComponent::OnComponentCopied(LightEntity entity)
	{
		auto& component = entity.GetComponent<SkylightComponent>();
		component.UpdateSceneLightData(entity.GetID());
	}

	void SkylightComponent::OnMemberChanged(LightEntity entity)
	{
		auto& component = entity.GetComponent<SkylightComponent>();
		component.UpdateSceneLightData(entity.GetID());
	}
}
