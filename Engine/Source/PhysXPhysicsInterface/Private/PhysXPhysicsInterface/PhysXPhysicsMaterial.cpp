#include "pxpch.h"

#include "PhysXPhysicsInterface/PhysXPhysicsMaterial.h"
#include "PhysXPhysicsInterface/PhysXPhysicsCore.h"

#include <PhysX/PxPhysicsAPI.h>

namespace Volt
{
	PhysXPhysicsMaterial::PhysXPhysicsMaterial(const PhysicsMaterialCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		auto& physXCore = PhysXPhysicsCore::GetInstance()->GetCore();
		m_material = physXCore.createMaterial(createInfo.staticFriction, createInfo.dynamicFriction, createInfo.bounciness);
	}

	PhysXPhysicsMaterial::~PhysXPhysicsMaterial()
	{
		if (m_material)
		{
			m_material->release();
		}

		m_material = nullptr;
	}
	
	void PhysXPhysicsMaterial::SetStaticFriction(float friction)
	{
		VT_ENSURE(m_material);
		m_createInfo.staticFriction = friction;
		
		m_material->setStaticFriction(friction);
	}
	
	void PhysXPhysicsMaterial::SetDynamicFriction(float friction)
	{
		VT_ENSURE(m_material);
		m_createInfo.dynamicFriction = friction;

		m_material->setDynamicFriction(friction);
	}
	
	void PhysXPhysicsMaterial::SetBounciness(float bounciness)
	{
		VT_ENSURE(m_material);
		m_createInfo.bounciness = bounciness;

		m_material->setRestitution(bounciness);
	}
}
