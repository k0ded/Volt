#pragma once

#include <PhysicsInterface/PhysicsMaterial.h>

#include <AssetSystem/Asset.h>

namespace Volt
{
	class PhysicsMaterialAsset : public Asset
	{
	public:
		PhysicsMaterialAsset();
		~PhysicsMaterialAsset() override;

	private:
		friend class PhysicsMaterialSerializer;

		Ref<PhysicsMaterial> m_material;
	};
}
