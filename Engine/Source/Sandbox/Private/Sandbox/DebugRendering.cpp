#include "sbpch.h"
#include "Sandbox.h"

#include "Sandbox/UserSettingsManager.h"
#include "Sandbox/ComponentVisualizers/ComponentVisualizerRegistry.h"
#include "Sandbox/Camera/EditorCameraController.h"
#include "Sandbox/Utility/EditorResources.h"
#include "Sandbox/Utility/EditorUtilities.h"

#include <Volt-Core/Algorithms.h>
#include <Volt-CoreComponents/RenderingComponents.h>
#include <Volt-CoreComponents/LightComponents.h>

#include <Volt-Renderer/Camera/Camera.h>

#include <EntitySystem/Scripting/CoreComponents.h>

#include <CoreUtilities/Profiling/Profiling.h>

void Sandbox::DrawEntityGizmos()
{
	VT_PROFILE_FUNCTION();

	const auto& settings = UserSettingsManager::GetSettings().sceneSettings;

	if (!settings.showGizmos || !m_runtimeScene || !m_runtimeScene->IsFinishedLoadingEntities())
	{
		return;
	}

	// There will be one gizmo per entity.
	const entt::registry& registry = m_runtimeScene->GetEntityScene().GetRegistry();
	auto entityView = registry.view<Volt::IDComponent>();

	const uint32_t numEntities = static_cast<uint32_t>(entityView.size());
	const glm::vec3 editorCameraPosition = m_editorCameraController->GetCamera()->GetPosition();
	const glm::mat4 editorCameraViewMatrix = m_editorCameraController->GetCamera()->GetView();

	Volt::Algo::ForEachParalellBlocking([&entityView, &editorCameraPosition, &editorCameraViewMatrix, scene = m_runtimeScene, &debugRenderer = m_debugRenderer](uint32_t threadIdx, uint32_t elementIdx)
	{
		VT_PROFILE_SCOPE("Entity");

		entt::entity entityHandle = *(entityView.begin() + elementIdx);
		Volt::Entity entity = scene->GetEntityFromHandle(entityHandle);

		const TQS entityTransform = entity.GetTransformTQS();

		constexpr float MaxDistance = 5000.f * 5000.f;
		constexpr float LerpStartDistance = 4000.f * 4000.f;
		constexpr float MaxScale = 1.f;
		constexpr float MinScale = 0.3f;

		const float distance = glm::distance2(editorCameraPosition, entityTransform.translation);

		float alpha = 1.f;
		if (distance >= LerpStartDistance)
		{
			alpha = glm::mix(1.f, 0.f, (distance - LerpStartDistance) / (MaxDistance - LerpStartDistance));
		}

		if (distance < MaxDistance)
		{
			float scale = glm::max(glm::min(distance / MaxDistance * 2.f, MaxScale), MinScale);

			EditorDrawInterface editorDrawInterface;

			EditorUtils::IterateComponentsInEntity(entity, [&editorDrawInterface](const VoltGUID& componentGuid) 
			{
				if (ComponentVisualizerRegistry::Get().HasComponentVisualizer(componentGuid))
				{
					auto componentVisualizer = ComponentVisualizerRegistry::Get().GetComponentVisualizer(componentGuid);
					componentVisualizer->DrawVisualization(editorDrawInterface);
				}
			});

			editorDrawInterface.Render(debugRenderer, editorCameraViewMatrix, entity.GetID(), entityTransform, scale, alpha);
		}

	}, numEntities, 128);
}
