#pragma once

#include "Volt-CoreComponents/Config.h"

#include <Volt-Assets/StreamingInstanceID.h>
#include <Volt-Animation/AnimationComponents.h>

#include <EntitySystem/ComponentRegistry.h>
#include <EntitySystem/Scripting/ECSAccessBuilder.h>

#include <AssetSystem/AssetTypes.h>
#include <AssetSystem/Asset_New.h>

namespace Volt
{
	class Camera;
	class ScenePrimitiveData;

	struct MeshComponent
	{
		using MeshEntity = ECS::Access
			::Write<MeshComponent>
			::Read<IDComponent>
			::WriteIfExists<AnimationPlayerComponent>
			::As<ECS::Type::Entity>;

		AssetHandle handle = Asset::Null();
		Vector<AssetHandle> materials;

		[[nodiscard]] inline const AssetHandle& GetHandle() const { return handle; }

		static void ReflectType(TypeDesc<MeshComponent>& reflect)
		{
			reflect.SetGUID("{45D008BE-65C9-4D6F-A0C6-377F7B384E47}"_guid);
			reflect.SetLabel("Mesh Component");
			reflect.AddMember(&MeshComponent::handle, "handle", "Mesh", "", Asset::Null(), AssetTypes::Mesh);
			reflect.AddMember(&MeshComponent::materials, "materials", "Materials", "", Asset::Null(), AssetTypes::Material);
			reflect.SetOnMemberChangedCallback(&MeshComponent::OnMemberChanged);
			reflect.SetOnInitializeCallback(&MeshComponent::OnIntitialize);
			reflect.SetOnDestroyCallback(&MeshComponent::OnDestroy);
			reflect.SetOnTransformChangedCallback(&MeshComponent::OnTransformChanged);
		}

		REGISTER_COMPONENT(MeshComponent);

		VTCC_API static void OnMemberChanged(MeshEntity entity);

	private:
		VTCC_API static void OnDestroy(MeshEntity entity);
		VTCC_API static void OnIntitialize(MeshEntity entity);
		VTCC_API static void OnTransformChanged(MeshEntity entity);

		Ref<ScenePrimitiveData> m_scenePrimitiveData;
		StreamingInstanceID m_streamingInstanceID;
	};

	struct CameraComponent
	{
		float fieldOfView = 60.f;
		float nearPlane = 1.f;
		float farPlane = 100'000.f;
		uint32_t priority = 0;

		Ref<Camera> camera;

		static void ReflectType(TypeDesc<CameraComponent>& reflect)
		{
			reflect.SetGUID("{9258BEEC-3A31-4CAB-AB1E-654524E1C398}"_guid);
			reflect.SetLabel("Camera Component");
			reflect.AddMember(&CameraComponent::fieldOfView, "fieldOfView", "Field Of View", "", 60.f);
			reflect.AddMember(&CameraComponent::nearPlane, "nearPlane", "Near Plane", "", 1.f);
			reflect.AddMember(&CameraComponent::farPlane, "farPlane", "Far Plane", "", 100'000.f);
			reflect.AddMember(&CameraComponent::priority, "priority", "Priority", "", 0);
			reflect.SetOnInitializeCallback(&CameraComponent::OnInitialize);
		}

		REGISTER_COMPONENT(CameraComponent);

	private:
		using CameraEntity = ECS::Access
			::Write<CameraComponent>
			::As<ECS::Type::Entity>;

		VTCC_API static void OnInitialize(CameraEntity entity);
	};

	struct TextRendererComponent
	{
		std::string text = "Text";
		AssetHandle font = Asset::Null();
		float maxWidth = 100.f;
		glm::vec4 color = { 1.f };

		static void ReflectType(TypeDesc<TextRendererComponent>& reflect)
		{
			reflect.SetGUID("{8AAA0646-40D2-47E6-B83F-72EA26BD8C01}"_guid);
			reflect.SetLabel("Text Renderer Component");
			reflect.AddMember(&TextRendererComponent::text, "text", "Text", "", std::string("Text"));
			reflect.AddMember(&TextRendererComponent::font, "font", "Font", "", Asset::Null(), AssetTypes::Font);
			reflect.AddMember(&TextRendererComponent::maxWidth, "maxWidth", "Max Width", "", 100.f);
			reflect.AddMember(&TextRendererComponent::color, "color", "Color", "", glm::vec4{ 1.f }, ComponentMemberFlag::Color4);
		}

		REGISTER_COMPONENT(TextRendererComponent);
	};

	struct SpriteComponent
	{
		AssetHandle materialHandle = Asset::Null();

		static void ReflectType(TypeDesc<SpriteComponent>& reflect)
		{
			reflect.SetGUID("{FDB47734-1B69-4558-B460-0975365DB400}"_guid);
			reflect.SetLabel("Sprite Component");
			reflect.AddMember(&SpriteComponent::materialHandle, "materialHandle", "Material", "", Asset::Null(), AssetTypes::Material);
		}

		REGISTER_COMPONENT(SpriteComponent);
	};

	struct VertexPaintedComponent
	{
		AssetHandle meshHandle = Asset::Null();
		Vector<uint32_t> vertexColors;

		static void ReflectType(TypeDesc<VertexPaintedComponent>& reflect)
		{
			reflect.SetGUID("{480B6514-05CB-4532-A366-B5DFD419E310}"_guid);
			reflect.SetLabel("Vertex Painted Component");
		}

		REGISTER_COMPONENT(VertexPaintedComponent);
	};

	struct PostProcessingStackComponent
	{
		AssetHandle postProcessingStack = Asset::Null();

		static void ReflectType(TypeDesc<PostProcessingStackComponent>& reflect)
		{
			reflect.SetGUID("{09340235-CDA0-496E-BEB5-A2F38BCE0033}"_guid);
			reflect.SetLabel("Post Processing Stack Component");
			reflect.AddMember(&PostProcessingStackComponent::postProcessingStack, "postProcessingStack", "Post Processing Stack", "", Asset::Null(), AssetTypes::PostProcessingStack);
		}

		REGISTER_COMPONENT(PostProcessingStackComponent);
	};

	struct DecalComponent
	{
		AssetHandle decalMaterial = Asset::Null();

		static void ReflectType(TypeDesc<DecalComponent>& reflect)
		{
			reflect.SetGUID("{09FA1C73-D508-4ADA-A101-A63703E91345}"_guid);
			reflect.SetLabel("Decal Component");
			reflect.AddMember(&DecalComponent::decalMaterial, "decalMaterial", "Material", "", Asset::Null(), AssetTypes::Material);
		}

		REGISTER_COMPONENT(DecalComponent);
	};
}
