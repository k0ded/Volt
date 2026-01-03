#include "vspch.h"

#include "Volt-Scene/Components/CoreComponents.h"

#include <Volt-CoreComponents/RenderingComponents.h>
#include <Volt-Renderer/Camera/Camera.h>

#include <EntitySystem/Scripting/CommonComponent.h>
#include <EntitySystem/Scripting/ECSBuilder.h>
#include <EntitySystem/Scripting/ECSSystemRegistry.h>
#include <EntitySystem/Scripting/CoreEnvironments.h>

namespace Volt
{
	using CommonEntity = ECS::Access
		::Write<CommonComponent>
		::As<ECS::Type::Entity>;

	void CommonSystem(CommonEntity entity, const env::VariableUpdate& variableUpdate)
	{
		entity.GetComponent<CommonComponent>().timeSinceCreation += variableUpdate.deltaTime;
	}

	using CameraEntity = ECS::Access
		::Write<CameraComponent>
		::Read<TransformComponent>
		::As<ECS::Type::Entity>;

	void CameraSystem(CameraEntity entity)
	{
		const auto& transform = entity.GetComponent<const TransformComponent>();

		if (!transform.visible)
		{
			return;
		}

		// #TODO_Ivar: This should be updated to include correct aspect ration and to use correct world position/rotation.
		auto& cameraComponent = entity.GetComponent<CameraComponent>();
		cameraComponent.camera->SetPerspectiveProjection(cameraComponent.fieldOfView, 16.f / 9.f, cameraComponent.nearPlane, cameraComponent.farPlane);
		cameraComponent.camera->SetPosition(entity.GetPosition());
		cameraComponent.camera->SetRotation(glm::eulerAngles(entity.GetRotation()));
	}

	void RegisterModule(ECSBuilder& builder)
	{
		builder.GetGameLoop(GameLoop::Variable).RegisterSystem(CommonSystem);
		builder.GetGameLoop(GameLoop::Variable).RegisterSystem(CameraSystem);
	}

	VT_REGISTER_ECS_MODULE(RegisterModule, "{33D9303D-D8E7-4A0B-933C-B4328E4F4BA2}"_guid);
}
