#pragma once

#include "Volt-CoreComponents/Config.h"

#include <Volt-Renderer/Texture/Texture2D.h>
#include <Volt-Renderer/RenderScene/SceneLightData.h>

#include <AssetSystem/Asset.h>

#include <EntitySystem/ComponentRegistry.h>
#include <EntitySystem/Scripting/ECSAccessBuilder.h>

#include <RHIModule/Images/Image.h>

#include <glm/glm.hpp>

namespace Volt
{
	struct PointLightComponent
	{
		float intensity = 1.f;
		float radius = 100.f;
		float falloff = 1.f;
		glm::vec3 color = { 1.f, 1.f, 1.f };
		bool castShadows = false;

		static void ReflectType(TypeDesc<PointLightComponent>& reflect)
		{
			reflect.SetGUID("{A30A8848-A30B-41DD-80F9-4E163C01ABC2}"_guid);
			reflect.SetLabel("Point Light Component");
			reflect.AddMember(&PointLightComponent::intensity, "intensity", "Intensity", "", 1.f);
			reflect.AddMember(&PointLightComponent::radius, "radius", "Radius", "", 100.f);
			reflect.AddMember(&PointLightComponent::falloff, "falloff", "Falloff", "", 1.f);
			reflect.AddMember(&PointLightComponent::color, "color", "Color", "", glm::vec3{ 1.f }, ComponentMemberFlag::Color3);
			reflect.AddMember(&PointLightComponent::castShadows, "castShadows", "Cast Shadows", "", false);
		
			reflect.SetOnMemberChangedCallback(&PointLightComponent::OnMemberChanged);
			reflect.SetOnComponentCopiedCallback(&PointLightComponent::OnComponentCopied);
			reflect.SetOnDestroyCallback(&PointLightComponent::OnDestroy);
			reflect.SetOnCreateCallback(&PointLightComponent::OnCreate);
			reflect.SetOnTransformChangedCallback(&PointLightComponent::OnTransformChanged);
		}

		REGISTER_COMPONENT(PointLightComponent);

	private:
		using LightEntity = ECS::Access
			::Write<PointLightComponent>
			::Read<IDComponent>
			::As<ECS::Type::Entity>;

		VTCC_API static void OnCreate(LightEntity entity);
		VTCC_API static void OnDestroy(LightEntity entity);
		VTCC_API static void OnTransformChanged(LightEntity entity);
		VTCC_API static void OnComponentCopied(LightEntity entity);
		VTCC_API static void OnMemberChanged(LightEntity entity);

		Ref<SceneLightData> m_sceneLightData;
	};

	struct SpotLightComponent
	{
		float intensity = 1.f;
		float outerAngle = 45.f;
		float innerAngle = 35.f;
		float range = 100.f;
		float falloff = 1.f;
		glm::vec3 color = { 1.f, 1.f, 1.f };
		bool castShadows = false;

		static void ReflectType(TypeDesc<SpotLightComponent>& reflect)
		{
			reflect.SetGUID("{D35F915F-53E5-4E15-AE5B-769F4D79B6F8}"_guid);
			reflect.SetLabel("Spot Light Component");
			reflect.AddMember(&SpotLightComponent::intensity, "intensity", "Intensity", "", 1.f);
			reflect.AddMember(&SpotLightComponent::innerAngle, "innerAngle", "Inner Angle", "", 35.f);
			reflect.AddMember(&SpotLightComponent::outerAngle, "outerAngle", "Outer Angle", "", 45.f);
			reflect.AddMember(&SpotLightComponent::range, "range", "Range", "", 100.f);
			reflect.AddMember(&SpotLightComponent::falloff, "falloff", "Falloff", "", 1.f);
			reflect.AddMember(&SpotLightComponent::color, "color", "Color", "", glm::vec3{ 1.f }, ComponentMemberFlag::Color3);
			reflect.AddMember(&SpotLightComponent::castShadows, "castShadows", "Cast Shadows", "", false);

			reflect.SetOnMemberChangedCallback(&SpotLightComponent::OnMemberChanged);
			reflect.SetOnComponentCopiedCallback(&SpotLightComponent::OnComponentCopied);
			reflect.SetOnDestroyCallback(&SpotLightComponent::OnDestroy);
			reflect.SetOnCreateCallback(&SpotLightComponent::OnCreate);
			reflect.SetOnTransformChangedCallback(&SpotLightComponent::OnTransformChanged);
		}

		REGISTER_COMPONENT(SpotLightComponent);

	private:
		using LightEntity = ECS::Access
			::Write<SpotLightComponent>
			::Read<IDComponent>
			::Read<TransformComponent>
			::As<ECS::Type::Entity>;

		VTCC_API static void OnCreate(LightEntity entity);
		VTCC_API static void OnDestroy(LightEntity entity);
		VTCC_API static void OnTransformChanged(LightEntity entity);
		VTCC_API static void OnComponentCopied(LightEntity entity);
		VTCC_API static void OnMemberChanged(LightEntity entity);

		Ref<SceneLightData> m_sceneLightData;
	};

	struct SphereLightComponent
	{
		float intensity = 1.f;
		float radius = 50.f;
		glm::vec3 color = { 1.f, 1.f, 1.f };

		static void ReflectType(TypeDesc<SphereLightComponent>& reflect)
		{
			reflect.SetGUID("{0D0CEEE2-A331-442A-BB4B-FBDB8E06C692}"_guid);
			reflect.SetLabel("Sphere Light Component");
			reflect.AddMember(&SphereLightComponent::intensity, "intensity", "Intensity", "", 1.f);
			reflect.AddMember(&SphereLightComponent::radius, "radius", "Radius", "", 50.f);
			reflect.AddMember(&SphereLightComponent::color, "color", "Color", "", glm::vec3{ 1.f }, ComponentMemberFlag::Color3);
		}

		REGISTER_COMPONENT(SphereLightComponent);
	};

	struct RectangleLightComponent
	{
		float intensity = 1.f;
		glm::vec3 color = { 1.f, 1.f, 1.f };
		float width = 50.f;
		float height = 50.f;

		static void ReflectType(TypeDesc<RectangleLightComponent>& reflect)
		{
			reflect.SetGUID("{5AEF9201-4A86-45F1-85F3-E95577E45BF2}"_guid);
			reflect.SetLabel("Rectangle Light Component");
			reflect.AddMember(&RectangleLightComponent::intensity, "intensity", "Intensity", "", 1.f);
			reflect.AddMember(&RectangleLightComponent::color, "color", "Color", "", glm::vec3{ 1.f }, ComponentMemberFlag::Color3);
			reflect.AddMember(&RectangleLightComponent::width, "width", "Width", "", 50.f);
			reflect.AddMember(&RectangleLightComponent::height, "height", "Height", "", 50.f);
		}

		REGISTER_COMPONENT(RectangleLightComponent);
	};

	struct DirectionalLightComponent
	{
		float intensity = 1.f;
		glm::vec3 color = { 1.f, 1.f, 1.f };
		float lightSize = 1.f;
		float sunRadius = 10.f;
		bool softShadows = true;
		bool castShadows = true;

		static void ReflectType(TypeDesc<DirectionalLightComponent>& reflect)
		{
			reflect.SetGUID("{EC5514FF-9DE7-44CA-BCD9-8A9F08883F59}"_guid);
			reflect.SetLabel("Directional Light Component");
			reflect.AddMember(&DirectionalLightComponent::intensity, "intensity", "Intensity", "", 1.f);
			reflect.AddMember(&DirectionalLightComponent::color, "color", "Color", "", glm::vec3{ 1.f }, ComponentMemberFlag::Color3);
			reflect.AddMember(&DirectionalLightComponent::lightSize, "lightSize", "Light Size", "", 1.f);
			reflect.AddMember(&DirectionalLightComponent::sunRadius, "sunRadius", "Sun Radius", "", 10.f);
			reflect.AddMember(&DirectionalLightComponent::softShadows, "softShadows", "Soft Shadows", "", true);
			reflect.AddMember(&DirectionalLightComponent::castShadows, "castShadows", "Cast Shadows", "", true);

			reflect.SetOnMemberChangedCallback(&DirectionalLightComponent::OnMemberChanged);
			reflect.SetOnComponentCopiedCallback(&DirectionalLightComponent::OnComponentCopied);
			reflect.SetOnDestroyCallback(&DirectionalLightComponent::OnDestroy);
			reflect.SetOnCreateCallback(&DirectionalLightComponent::OnCreate);
			reflect.SetOnTransformChangedCallback(&DirectionalLightComponent::OnTransformChanged);
		}

		REGISTER_COMPONENT(DirectionalLightComponent);

	private:
		using LightEntity = ECS::Access
			::Write<DirectionalLightComponent>
			::Read<IDComponent>
			::Read<TransformComponent>
			::As<ECS::Type::Entity>;

		VTCC_API static void OnCreate(LightEntity entity);
		VTCC_API static void OnDestroy(LightEntity entity);
		VTCC_API static void OnTransformChanged(LightEntity entity);
		VTCC_API static void OnComponentCopied(LightEntity entity);
		VTCC_API static void OnMemberChanged(LightEntity entity);

		Ref<SceneLightData> m_sceneLightData;
	};

	struct SkylightComponent
	{
		struct Environment
		{
			RefPtr<RHI::Image> diffuse;
			RefPtr<RHI::Image> specular;
		};

		AssetHandle environmentTextureHandle = Asset::Null();
		bool show = true;
		float lod = 0.f;
		float intensity = 1.f;

		Environment currentSceneEnvironment;
		AssetHandle lastEnvironmentHandle = Asset::Null();

		static void ReflectType(TypeDesc<SkylightComponent>& reflect)
		{
			reflect.SetGUID("{29F75381-2873-4734-A074-3F3640E54C84}"_guid);
			reflect.SetLabel("Skylight Component");
			reflect.AddMember(&SkylightComponent::environmentTextureHandle, "environmentHandle", "Environment", "", Asset::Null(), AssetTypes::Texture);
			reflect.AddMember(&SkylightComponent::intensity, "intensity", "Intensity", "", 1.f);
			reflect.AddMember(&SkylightComponent::lod, "lod", "LOD", "", 0.f);
			reflect.AddMember(&SkylightComponent::show, "show", "Show", "", true);

			reflect.SetOnMemberChangedCallback(&SkylightComponent::OnMemberChanged);
			reflect.SetOnComponentCopiedCallback(&SkylightComponent::OnComponentCopied);
			reflect.SetOnDestroyCallback(&SkylightComponent::OnDestroy);
			reflect.SetOnCreateCallback(&SkylightComponent::OnCreate);
		}

		REGISTER_COMPONENT(SkylightComponent);

	private:
		using LightEntity = ECS::Access
			::Write<SkylightComponent>
			::Read<IDComponent>
			::As<ECS::Type::Entity>;

		VTCC_API static void OnCreate(LightEntity entity);
		VTCC_API static void OnDestroy(LightEntity entity);
		VTCC_API static void OnComponentCopied(LightEntity entity);
		VTCC_API static void OnMemberChanged(LightEntity entity);

		Ref<SceneLightData> m_sceneLightData;
	};
}
