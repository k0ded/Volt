#pragma once

#include "PhysicsInterface/PhysicsTypes.h"
#include "PhysicsInterface/PhysicsHandleType.h"
#include "PhysicsInterface/PhysicsLayer.h"

#include <CoreUtilities/Pointers/Ref.h>

#include <glm/glm.hpp>

namespace Volt
{
	class PhysicsMaterial;
	class PhysicsActor;

	struct ColliderCreateInfoCommon
	{
		glm::vec3 offset;
		glm::vec3 scale;
		bool isTrigger = false;
		Ref<PhysicsMaterial> physicalMaterial;
		
		PhysicsActor* targetActor = nullptr;
	};

	struct BoxColliderCreateInfo : public ColliderCreateInfoCommon
	{
		glm::vec3 halfSize = 50.f;
	};

	struct SphereColliderCreateInfo : public ColliderCreateInfoCommon
	{
		float radius = 50.f;
	};

	struct CapsuleColliderCreateInfo : public ColliderCreateInfoCommon
	{
		float radius = 50.f;
		float height = 100.f;
	};

	struct ConvexMeshColliderCreateInfo : public ColliderCreateInfoCommon
	{
	};

	struct TriangleMeshColliderCreateInfo : public ColliderCreateInfoCommon
	{

	};

	class ColliderShape : public PhysicsHandleType
	{
	public:
		virtual ~ColliderShape() = default;

		virtual void SetIsTrigger(bool isTrigger) = 0;
		virtual void SetOffset(const glm::vec3& offset) = 0;
		virtual void SetScale(const glm::vec3& scale) = 0;
		virtual void AssignToPhysicsLayer(PhysicsLayerID layerId) = 0;

		virtual Ref<PhysicsMaterial> GetPhysicalMaterial() const = 0;
		virtual ColliderType GetColliderType() const = 0;
		virtual const glm::vec3& GetOffset() const = 0;
		virtual const glm::vec3& GetScale() const = 0;
		virtual bool IsTrigger() const = 0;

		virtual void DetachFromActor() = 0;
	};

	class BoxColliderShape : public ColliderShape
	{
	public:
		virtual void SetHalfSize(const glm::vec3& halfSize) = 0;
		virtual const glm::vec3& GetHalfSize() const = 0;
	};

	class SphereColliderShape : public ColliderShape
	{
	public:
		virtual void SetRadius(float radius) = 0;
		virtual float GetRadius() const = 0;
	};

	class CapsuleColliderShape : public ColliderShape
	{
	public:
		virtual void SetHeight(float height) = 0;
		virtual void SetRadius(float radius) = 0;

		virtual float GetHeight() const = 0;
		virtual float GetRadius() const = 0;
	};

	class ConvexMeshShape : public ColliderShape
	{
	public:

	};

	class TriangleMeshShape : public ColliderShape
	{
	public:
	};
}
