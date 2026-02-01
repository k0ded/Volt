#pragma once

#include "Volt-CoreComponents/Config.h"

#include <Volt-Renderer/Texture/Texture2D.h>
#include <Volt-Renderer/RenderScene/SceneLightData.h>

#include <Volt-Assets/StreamingInstanceID.h>

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
			reflect.AddMember(&PointLightComponent::intensity, 'inte', "Intensity", "", 1.f);
			reflect.AddMember(&PointLightComponent::radius, 'radi', "Radius", "", 100.f);
			reflect.AddMember(&PointLightComponent::falloff, 'fall', "Falloff", "", 1.f);
			reflect.AddMember(&PointLightComponent::color, 'col', "Color", "", glm::vec3{1.f}, ComponentMemberFlag::Color3);
			reflect.AddMember(&PointLightComponent::castShadows, 'shdw', "Cast Shadows", "", false);
		
			reflect.SetOnMemberChangedCallback(&PointLightComponent::OnMemberChanged);
			reflect.SetOnDestroyCallback(&PointLightComponent::OnDestroy);
			reflect.SetOnInitializeCallback(&PointLightComponent::OnInitialize);
			reflect.SetOnTransformChangedCallback(&PointLightComponent::OnTransformChanged);
		}

	private:
		using LightEntity = ECS::Access
			::Write<PointLightComponent>
			::Read<IDComponent>
			::As<ECS::Type::Entity>;

		VTCC_API static void OnInitialize(LightEntity entity);
		VTCC_API static void OnDestroy(LightEntity entity);
		VTCC_API static void OnTransformChanged(LightEntity entity);
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
			reflect.AddMember(&SpotLightComponent::intensity, 'inte', "Intensity", "", 1.f);
			reflect.AddMember(&SpotLightComponent::innerAngle, 'angi', "Inner Angle", "", 35.f);
			reflect.AddMember(&SpotLightComponent::outerAngle, 'outi', "Outer Angle", "", 45.f);
			reflect.AddMember(&SpotLightComponent::range, 'rang', "Range", "", 100.f);
			reflect.AddMember(&SpotLightComponent::falloff, 'fall', "Falloff", "", 1.f);
			reflect.AddMember(&SpotLightComponent::color, 'col', "Color", "", glm::vec3{1.f}, ComponentMemberFlag::Color3);
			reflect.AddMember(&SpotLightComponent::castShadows, 'shdw', "Cast Shadows", "", false);

			reflect.SetOnMemberChangedCallback(&SpotLightComponent::OnMemberChanged);
			reflect.SetOnDestroyCallback(&SpotLightComponent::OnDestroy);
			reflect.SetOnInitializeCallback(&SpotLightComponent::OnInitialize);
			reflect.SetOnTransformChangedCallback(&SpotLightComponent::OnTransformChanged);
		}

	private:
		using LightEntity = ECS::Access
			::Write<SpotLightComponent>
			::Read<IDComponent>
			::Read<TransformComponent>
			::As<ECS::Type::Entity>;

		VTCC_API static void OnInitialize(LightEntity entity);
		VTCC_API static void OnDestroy(LightEntity entity);
		VTCC_API static void OnTransformChanged(LightEntity entity);
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
			reflect.AddMember(&SphereLightComponent::intensity, 'inte', "Intensity", "", 1.f);
			reflect.AddMember(&SphereLightComponent::radius, 'radi', "Radius", "", 50.f);
			reflect.AddMember(&SphereLightComponent::color, 'col', "Color", "", glm::vec3{1.f}, ComponentMemberFlag::Color3);
		}
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
			reflect.AddMember(&RectangleLightComponent::intensity, 'inte', "Intensity", "", 1.f);
			reflect.AddMember(&RectangleLightComponent::color, 'col', "Color", "", glm::vec3{1.f}, ComponentMemberFlag::Color3);
			reflect.AddMember(&RectangleLightComponent::width, 'wid', "Width", "", 50.f);
			reflect.AddMember(&RectangleLightComponent::height, 'heig', "Height", "", 50.f);
		}
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
			reflect.AddMember(&DirectionalLightComponent::intensity, 'inte', "Intensity", "", 1.f);
			reflect.AddMember(&DirectionalLightComponent::color, 'col', "Color", "", glm::vec3{1.f}, ComponentMemberFlag::Color3);
			reflect.AddMember(&DirectionalLightComponent::lightSize, 'lisz', "Light Size", "", 1.f);
			reflect.AddMember(&DirectionalLightComponent::sunRadius, 'snrd', "Sun Radius", "", 10.f);
			reflect.AddMember(&DirectionalLightComponent::softShadows, 'sfsh', "Soft Shadows", "", true);
			reflect.AddMember(&DirectionalLightComponent::castShadows, 'shdw', "Cast Shadows", "", true);

			reflect.SetOnMemberChangedCallback(&DirectionalLightComponent::OnMemberChanged);
			reflect.SetOnDestroyCallback(&DirectionalLightComponent::OnDestroy);
			reflect.SetOnInitializeCallback(&DirectionalLightComponent::OnInitialize);
			reflect.SetOnTransformChangedCallback(&DirectionalLightComponent::OnTransformChanged);
		}

	private:
		using LightEntity = ECS::Access
			::Write<DirectionalLightComponent>
			::Read<IDComponent>
			::Read<TransformComponent>
			::As<ECS::Type::Entity>;

		VTCC_API static void OnInitialize(LightEntity entity);
		VTCC_API static void OnDestroy(LightEntity entity);
		VTCC_API static void OnTransformChanged(LightEntity entity);
		VTCC_API static void OnMemberChanged(LightEntity entity);

		Ref<SceneLightData> m_sceneLightData;
	};

	struct SkylightComponent
	{
		using LightEntity = ECS::Access
			::Write<SkylightComponent>
			::Read<IDComponent>
			::As<ECS::Type::Entity>;

		AssetHandle environmentTextureHandle = Asset::Null();
		bool show = true;
		float lod = 0.f;
		float intensity = 1.f;

		static void ReflectType(TypeDesc<SkylightComponent>& reflect)
		{
			reflect.SetGUID("{29F75381-2873-4734-A074-3F3640E54C84}"_guid);
			reflect.SetLabel("Skylight Component");
			reflect.AddMember(&SkylightComponent::environmentTextureHandle, 'env', "Environment", "", Asset::Null(), AssetTypes::EnvironmentTexture);
			reflect.AddMember(&SkylightComponent::intensity, 'inte', "Intensity", "", 1.f);
			reflect.AddMember(&SkylightComponent::lod, 'lod', "LOD", "", 0.f);
			reflect.AddMember(&SkylightComponent::show, 'show', "Show", "", true);

			reflect.SetOnMemberChangedCallback(&SkylightComponent::OnMemberChanged);
			reflect.SetOnDestroyCallback(&SkylightComponent::OnDestroy);
			reflect.SetOnInitializeCallback(&SkylightComponent::OnInitialize);
		}

		VTCC_API static void OnMemberChanged(LightEntity entity);

	private:

		VTCC_API static void OnInitialize(LightEntity entity);
		VTCC_API static void OnDestroy(LightEntity entity);

		void UpdateSceneLightData(EntityID entityId);

		Ref<SceneLightData> m_sceneLightData;
		StreamingInstanceID m_streamingInstanceID;
	};
}
