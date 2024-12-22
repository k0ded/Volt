#pragma once

#include <PhysicsInterface/ColliderShape.h>

namespace physx
{
	class PxShape;
}

namespace Volt
{
	class PhysXBoxColliderShape : public BoxColliderShape
	{
	public:
		PhysXBoxColliderShape(const BoxColliderCreateInfo& createInfo);
		~PhysXBoxColliderShape() override;

		void SetIsTrigger(bool isTrigger) override;
		void SetOffset(const glm::vec3& offset) override;
		void SetScale(const glm::vec3& scale) override;
		void SetHalfSize(const glm::vec3& halfSize) override;
		void AssignToPhysicsLayer(PhysicsLayerID layerId) override;

		inline Ref<PhysicsMaterial> GetPhysicalMaterial() const override { return m_createInfo.physicalMaterial; }
		inline ColliderType GetColliderType() const override { return ColliderType::Sphere; }
		inline const glm::vec3& GetOffset() const override { return m_createInfo.offset; }
		inline const glm::vec3& GetScale() const override { return m_createInfo.scale; }
		inline const glm::vec3& GetHalfSize() const override { return m_createInfo.halfSize; }
		inline bool IsTrigger() const override { return m_createInfo.isTrigger; }

		void DetachFromActor() override;

	protected:
		inline void* GetHandleImpl() const { return m_shape; }

	private:
		BoxColliderCreateInfo m_createInfo;
		physx::PxShape* m_shape = nullptr;
	};

	class PhysXSphereColliderShape : public SphereColliderShape
	{
	public:
		PhysXSphereColliderShape(const SphereColliderCreateInfo& createInfo);
		~PhysXSphereColliderShape() override;

		void SetIsTrigger(bool isTrigger) override;
		void SetOffset(const glm::vec3& offset) override;
		void SetScale(const glm::vec3& scale) override;
		void SetRadius(float radius) override;
		void AssignToPhysicsLayer(PhysicsLayerID layerId) override;

		inline Ref<PhysicsMaterial> GetPhysicalMaterial() const override { return m_createInfo.physicalMaterial; }
		inline ColliderType GetColliderType() const override { return ColliderType::Sphere; }
		inline const glm::vec3& GetOffset() const override { return m_createInfo.offset; }
		inline const glm::vec3& GetScale() const override { return m_createInfo.scale; }
		inline float GetRadius() const override { return m_createInfo.radius; }
		inline bool IsTrigger() const override { return m_createInfo.isTrigger; }

		void DetachFromActor() override;

	protected:
		inline void* GetHandleImpl() const { return m_shape; }

	private:
		SphereColliderCreateInfo m_createInfo;
		physx::PxShape* m_shape = nullptr;
	};

	class PhysXCapsuleColliderShape : public CapsuleColliderShape
	{
	public:
		PhysXCapsuleColliderShape(const CapsuleColliderCreateInfo& createInfo);
		~PhysXCapsuleColliderShape() override;

		void SetIsTrigger(bool isTrigger) override;
		void SetOffset(const glm::vec3& offset) override;
		void SetScale(const glm::vec3& scale) override;
		void SetRadius(float radius) override;
		void SetHeight(float height) override;
		void AssignToPhysicsLayer(PhysicsLayerID layerId) override;

		inline Ref<PhysicsMaterial> GetPhysicalMaterial() const override { return m_createInfo.physicalMaterial; }
		inline ColliderType GetColliderType() const override { return ColliderType::Sphere; }
		inline const glm::vec3& GetOffset() const override { return m_createInfo.offset; }
		inline const glm::vec3& GetScale() const override { return m_createInfo.scale; }
		inline float GetRadius() const override { return m_createInfo.radius; }
		inline float GetHeight() const override { return m_createInfo.height; }
		inline bool IsTrigger() const override { return m_createInfo.isTrigger; }

		void DetachFromActor() override;

	protected:
		inline void* GetHandleImpl() const { return m_shape; }

	private:
		CapsuleColliderCreateInfo m_createInfo;
		physx::PxShape* m_shape = nullptr;
	};
}
