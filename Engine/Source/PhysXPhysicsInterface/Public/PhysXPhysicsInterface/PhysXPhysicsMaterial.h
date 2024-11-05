#pragma once

#include <PhysicsInterface/PhysicsMaterial.h>

namespace physx
{
	class PxMaterial;
}

namespace Volt
{
	class PhysXPhysicsMaterial : public PhysicsMaterial
	{
	public:
		PhysXPhysicsMaterial(const PhysicsMaterialCreateInfo& createInfo);
		~PhysXPhysicsMaterial() override;

		void SetStaticFriction(float friction) override;
		void SetDynamicFriction(float friction) override;
		void SetBounciness(float bounciness) override;

		VT_INLINE float GetStaticFriction() const override { return m_createInfo.staticFriction; }
		VT_INLINE float GetDynamicFriction() const override { return m_createInfo.dynamicFriction; }
		VT_INLINE float GetBounciness() const override { return m_createInfo.bounciness; }

	protected:
		VT_INLINE void* GetHandleImpl() const override { return m_material; }

	private:
		PhysicsMaterialCreateInfo m_createInfo;

		physx::PxMaterial* m_material = nullptr;
	};
}
