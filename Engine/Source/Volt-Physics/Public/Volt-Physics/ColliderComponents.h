#pragma once

#include "Volt-Physics/PhysicsMaterialAsset.h"
#include "Volt-Physics/Config.h"

#include <AssetSystem/AssetTypes.h>

#include <EntitySystem/Scripting/ECSAccessBuilder.h>
#include <PhysicsInterface/PhysicsTypes.h>

#include <AssetSystem/Asset.h>

namespace Volt
{
	struct BoxColliderComponent
	{
		glm::vec3 halfSize = { 50.f, 50.f, 50.f };
		glm::vec3 offset = { 0.f, 0.f, 0.f };
		bool isTrigger = false;
		AssetHandle material = Asset::Null();

		PhysicsColliderID colliderId;

		inline BoxColliderComponent(const glm::vec3& aHalfSize = { 50.f, 50.f, 50.f }, const glm::vec3& aOffset = { 0.f }, bool aIsTrigger = false, AssetHandle aMaterial = Asset::Null())
			: halfSize(aHalfSize), offset(aOffset), isTrigger(aIsTrigger), material(aMaterial)
		{
		}

		static void ReflectType(TypeDesc<BoxColliderComponent>& reflect)
		{
			reflect.SetGUID("{29707475-D536-4DA4-8D3A-A98948C89A5}"_guid);
			reflect.SetLabel("Box Collider Component");
			reflect.AddMember(&BoxColliderComponent::halfSize, 'hasi', "Half Size", "", glm::vec3{50.f});
			reflect.AddMember(&BoxColliderComponent::offset, 'offs', "Offset", "", glm::vec3{0.f});
			reflect.AddMember(&BoxColliderComponent::isTrigger, 'istr', "Is Trigger", "", false);
			reflect.AddMember(&BoxColliderComponent::material, 'mat', "Material", "", Asset::Null(), AssetTypes::PhysicsMaterial);
			reflect.SetOnInitializeCallback(&BoxColliderComponent::OnInitialize);
			reflect.SetOnDestroyCallback(&BoxColliderComponent::OnDestroy);
		}

	private:
		using PhysicsEntity = ECS::Access
			::Write<BoxColliderComponent>
			::As<ECS::Type::Entity>;

		VTP_API static void OnInitialize(PhysicsEntity entity);
		VTP_API static void OnDestroy(PhysicsEntity entity);
	};

	struct SphereColliderComponent
	{
		float radius = 50.f;
		glm::vec3 offset = { 0.f, 0.f, 0.f };
		bool isTrigger = false;
		AssetHandle material = Asset::Null();

		PhysicsColliderID colliderId;

		inline SphereColliderComponent(float aRadius = 50.f, const glm::vec3& aOffset = { 0.f }, bool aIsTrigger = false, AssetHandle aMaterial = Asset::Null())
			: radius(aRadius), offset(aOffset), isTrigger(aIsTrigger), material(aMaterial)
		{
		}

		static void ReflectType(TypeDesc<SphereColliderComponent>& reflect)
		{
			reflect.SetGUID("{90246BCE-FF83-41A2-A076-AB0A947C0D6A}"_guid);
			reflect.SetLabel("Sphere Collider Component");
			reflect.AddMember(&SphereColliderComponent::radius, 'radi', "Radius", "", 50.f);
			reflect.AddMember(&SphereColliderComponent::offset, 'offs', "Offset", "", glm::vec3{0.f});
			reflect.AddMember(&SphereColliderComponent::isTrigger, 'istr', "Is Trigger", "", false);
			reflect.AddMember(&SphereColliderComponent::material, 'mat', "Material", "", Asset::Null(), AssetTypes::PhysicsMaterial);
			reflect.SetOnInitializeCallback(&SphereColliderComponent::OnInitialize);
			reflect.SetOnDestroyCallback(&SphereColliderComponent::OnDestroy);
		}

	private:
		using PhysicsEntity = ECS::Access
			::Write<SphereColliderComponent>
			::As<ECS::Type::Entity>;

		VTP_API static void OnInitialize(PhysicsEntity entity);
		VTP_API static void OnDestroy(PhysicsEntity entity);
	};

	struct CapsuleColliderComponent
	{
		float radius = 50.f;
		float height = 50.f;
		glm::vec3 offset = { 0.f, 0.f, 0.f };
		bool isTrigger = false;
		AssetHandle material = Asset::Null();

		PhysicsColliderID colliderId;

		inline CapsuleColliderComponent(float aRadius = 50.f, float aHeight = 50.f, const glm::vec3& aOffset = { 0.f }, bool aIsTrigger = false, AssetHandle aMaterial = Asset::Null())
			: radius(aRadius), height(aHeight), offset(aOffset), isTrigger(aIsTrigger), material(aMaterial)
		{
		}

		static void ReflectType(TypeDesc<CapsuleColliderComponent>& reflect)
		{
			reflect.SetGUID("{54A48952-7A77-492B-8A9C-2440D82EE5E2}"_guid);
			reflect.SetLabel("Capsule Collider Component");
			reflect.AddMember(&CapsuleColliderComponent::radius, 'radi', "Radius", "", 50.f);
			reflect.AddMember(&CapsuleColliderComponent::height, 'heig', "Height", "", 50.f);
			reflect.AddMember(&CapsuleColliderComponent::offset, 'offs', "Offset", "", glm::vec3{0.f});
			reflect.AddMember(&CapsuleColliderComponent::isTrigger, 'istr', "Is Trigger", "", false);
			reflect.AddMember(&CapsuleColliderComponent::material, 'mat', "Material", "", Asset::Null(), AssetTypes::PhysicsMaterial);
			reflect.SetOnInitializeCallback(&CapsuleColliderComponent::OnInitialize);
			reflect.SetOnDestroyCallback(&CapsuleColliderComponent::OnDestroy);
		}

	private:
		using PhysicsEntity = ECS::Access
			::Write<CapsuleColliderComponent>
			::As<ECS::Type::Entity>;

		VTP_API static void OnInitialize(PhysicsEntity entity);
		VTP_API static void OnDestroy(PhysicsEntity entity);
	};

	struct MeshColliderComponent
	{
		AssetHandle colliderMesh = Asset::Null();
		AssetHandle material = Asset::Null();
		int32_t subMeshIndex = -1;
		bool isConvex = true;
		bool isTrigger = false;

		PhysicsColliderID colliderId;

		inline MeshColliderComponent(AssetHandle aColliderMesh = Asset::Null(), bool aIsConvex = false, bool aIsTrigger = false, AssetHandle aMaterial = Asset::Null(), int32_t aSubMeshIndex = -1)
			: colliderMesh(aColliderMesh), material(aMaterial), subMeshIndex(aSubMeshIndex), isConvex(aIsConvex), isTrigger(aIsTrigger)
		{
		}

		static void ReflectType(TypeDesc<MeshColliderComponent>& reflect)
		{
			reflect.SetGUID("{E709C708-ED3C-4F68-BC1D-2FE32B897722}"_guid);
			reflect.SetLabel("Mesh Collider Component");
			reflect.AddMember(&MeshColliderComponent::colliderMesh, 'clme', "Collider Mesh", "", Asset::Null(), AssetTypes::Mesh);
			reflect.AddMember(&MeshColliderComponent::material, 'mat', "Material", "", Asset::Null(), AssetTypes::PhysicsMaterial);
			reflect.AddMember(&MeshColliderComponent::subMeshIndex, 'smi', "subMeshIndex", "", -1);
			reflect.AddMember(&MeshColliderComponent::isConvex, 'isco', "Is Convex", "", true);
			reflect.AddMember(&MeshColliderComponent::isTrigger, 'istr', "Is Trigger", "", false);
			reflect.SetOnInitializeCallback(&MeshColliderComponent::OnInitialize);
			reflect.SetOnDestroyCallback(&MeshColliderComponent::OnDestroy);
		}

	private:
		using PhysicsEntity = ECS::Access
			::Write<MeshColliderComponent>
			::As<ECS::Type::Entity>;

		VTP_API static void OnInitialize(PhysicsEntity entity);
		VTP_API static void OnDestroy(PhysicsEntity entity);
	};
}
