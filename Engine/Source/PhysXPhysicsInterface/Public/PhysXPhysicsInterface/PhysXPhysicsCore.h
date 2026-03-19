#pragma once

#include <PhysicsInterface/PhysicsCore.h>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Pointers/Unique.h>

namespace physx
{
	class PxFoundation;
	class PxDefaultCpuDispatcher;
	class PxPhysics;

	class PxDefaultAllocator;
}

namespace Volt
{
	class PhysXDebugger;
	class PhysXContactListener;

	struct PhysicsMaterialCreateInfo;

	class PhysXPhysicsCore : public PhysicsCore
	{
	public:
		PhysXPhysicsCore(const PhysicsCoreCreateInfo& createInfo);
		~PhysXPhysicsCore() override;

		Ref<PhysicsScene> CreateScene(const PhysicsSceneCreateInfo& createInfo) const override;
		Ref<PhysicsMaterial> CreateMaterial(const PhysicsMaterialCreateInfo& createInfo) const override;

		VT_NODISCARD VT_INLINE physx::PxPhysics& GetCore() const { return *m_physics; }
		VT_NODISCARD VT_INLINE physx::PxFoundation& GetFoundation() const { return *m_foundation; }
		VT_NODISCARD VT_INLINE physx::PxDefaultCpuDispatcher* GetCPUDispatcher() const { return m_defaultCPUDispatcher; }
		VT_NODISCARD VT_INLINE PhysXContactListener& GetContactListener() const { return *m_physXContactListener; }

		VT_NODISCARD VT_INLINE static PhysXPhysicsCore* GetInstance() { return s_instance; }

	private:
		inline static PhysXPhysicsCore* s_instance = nullptr;

		physx::PxPhysics* m_physics = nullptr;
		physx::PxFoundation* m_foundation = nullptr;
		physx::PxDefaultCpuDispatcher* m_defaultCPUDispatcher = nullptr;

		Unique<physx::PxDefaultAllocator> m_physXAllocator;
		Unique<PhysXDebugger> m_physXDebugger;

		Ref<ContactListener> m_contactListener;
		Unique<PhysXContactListener> m_physXContactListener;
	};
}
