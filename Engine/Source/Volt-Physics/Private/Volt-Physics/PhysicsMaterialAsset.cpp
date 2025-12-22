#include "vppch.h"
#include "Volt-Physics/PhysicsMaterialAsset.h"

#include <Volt-Physics/PhysicsSubSystem.h>

#include <SubSystem/SubSystemManager.h>

namespace Volt
{
	PhysicsMaterialAsset::PhysicsMaterialAsset()
	{
	}

	PhysicsMaterialAsset::~PhysicsMaterialAsset()
	{
	}

	void PhysicsMaterialAsset::Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata)
	{
		if (archive.IsLoading())
		{
			PhysicsMaterialCreateInfo materialCreateInfo{};
		
			archive << materialCreateInfo.staticFriction;
			archive << materialCreateInfo.dynamicFriction;
			archive << materialCreateInfo.bounciness;
		
			m_material = SubSystemManager::GetSubSystem<PhysicsSubSystem>()->GetPhysicsCore()->CreateMaterial(materialCreateInfo);
		}
		else
		{
			float staticFriction = m_material->GetStaticFriction();
			float dynamicFriction = m_material->GetDynamicFriction();
			float bounciness = m_material->GetBounciness();

			archive << staticFriction;
			archive << dynamicFriction;
			archive << bounciness;
		}
	}
}
