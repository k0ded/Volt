#pragma once

#include "Volt-CoreComponents/Config.h"

#include <Volt-Assets/StreamingInstanceID.h>
#include <Volt-Animation/AnimationComponents.h>

#include <EntitySystem/ComponentRegistry.h>
#include <EntitySystem/Scripting/ECSAccessBuilder.h>

#include <AssetSystem/AssetTypes.h>
#include <AssetSystem/Asset.h>

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

		VTCC_API void SetMesh(AssetHandle meshHandle, EntityID owningEntityId);

		static void ReflectType(TypeDesc<MeshComponent>& reflect)
		{
			reflect.SetGUID("{45D008BE-65C9-4D6F-A0C6-377F7B384E47}"_guid);
			reflect.SetLabel("Mesh Component");
			reflect.AddMember(&MeshComponent::handle, 'hndl', "Mesh", "", Asset::Null(), AssetTypes::Mesh);
			reflect.AddMember(&MeshComponent::materials, 'mats', "Materials", "", Asset::Null(), AssetTypes::Material);
			reflect.SetOnMemberChangedCallback(&MeshComponent::OnMemberChanged);
			reflect.SetOnInitializeCallback(&MeshComponent::OnIntitialize);
			reflect.SetOnDestroyCallback(&MeshComponent::OnDestroy);
			reflect.SetOnTransformChangedCallback(&MeshComponent::OnTransformChanged);
		} 

		VTCC_API static void OnMemberChanged(MeshEntity entity);

	private:
		VTCC_API static void OnDestroy(MeshEntity entity);
		VTCC_API static void OnIntitialize(MeshEntity entity);
		VTCC_API static void OnTransformChanged(MeshEntity entity);

		VTCC_API void UpdateMesh();

		Ref<ScenePrimitiveData> m_scenePrimitiveData;
		StreamingInstanceID m_streamingInstanceID;
		AssetHandle m_prevHandle = Asset::Null();
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
			reflect.AddMember(&CameraComponent::fieldOfView, 'fov', "Field Of View", "", 60.f);
			reflect.AddMember(&CameraComponent::nearPlane, 'nrpl', "Near Plane", "", 1.f);
			reflect.AddMember(&CameraComponent::farPlane, 'frpl', "Far Plane", "", 100'000.f);
			reflect.AddMember(&CameraComponent::priority, 'prio', "Priority", "", 0);
			reflect.SetOnInitializeCallback(&CameraComponent::OnInitialize);
		}

	private:
		using CameraEntity = ECS::Access
			::Write<CameraComponent>
			::As<ECS::Type::Entity>;

		VTCC_API static void OnInitialize(CameraEntity entity);
	};

	struct TextRendererComponent
	{
		String text = "Text";
		AssetHandle font = Asset::Null();
		float maxWidth = 100.f;
		glm::vec4 color = { 1.f };

		static void ReflectType(TypeDesc<TextRendererComponent>& reflect)
		{
			reflect.SetGUID("{8AAA0646-40D2-47E6-B83F-72EA26BD8C01}"_guid);
			reflect.SetLabel("Text Renderer Component");
			reflect.AddMember(&TextRendererComponent::text, 'text', "Text", "", String("Text"));
			reflect.AddMember(&TextRendererComponent::font, 'font', "Font", "", Asset::Null(), AssetTypes::Font);
			reflect.AddMember(&TextRendererComponent::maxWidth, 'mxwd', "Max Width", "", 100.f);
			reflect.AddMember(&TextRendererComponent::color, 'col', "Color", "", glm::vec4{1.f}, ComponentMemberFlag::Color4);
		}
	};

	struct SpriteComponent
	{
		AssetHandle materialHandle = Asset::Null();

		static void ReflectType(TypeDesc<SpriteComponent>& reflect)
		{
			reflect.SetGUID("{FDB47734-1B69-4558-B460-0975365DB400}"_guid);
			reflect.SetLabel("Sprite Component");
			reflect.AddMember(&SpriteComponent::materialHandle, 'hndl', "Material", "", Asset::Null(), AssetTypes::Material);
		}
	};

	struct DecalComponent
	{
		AssetHandle decalMaterial = Asset::Null();

		static void ReflectType(TypeDesc<DecalComponent>& reflect)
		{
			reflect.SetGUID("{09FA1C73-D508-4ADA-A101-A63703E91345}"_guid);
			reflect.SetLabel("Decal Component");
			reflect.AddMember(&DecalComponent::decalMaterial, 'dcl', "Material", "", Asset::Null(), AssetTypes::Material);
		}
	};
}
