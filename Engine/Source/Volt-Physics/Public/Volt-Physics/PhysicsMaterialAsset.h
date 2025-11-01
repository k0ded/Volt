#pragma once

#include "Volt-Physics/Config.h"

#include <AssetSystem/AssetTypes.h>

#include <PhysicsInterface/PhysicsMaterial.h>

#include <AssetSystem/Asset_New.h>

namespace Volt
{
	class PhysicsMaterialAsset : public Asset
	{
	public:
		PhysicsMaterialAsset();
		~PhysicsMaterialAsset() override;

		static AssetType GetStaticType() { return AssetTypes::PhysicsMaterial; }
		AssetType GetType() const override { return GetStaticType(); };
		uint32_t GetVersion() const override { return 1; }

		Ref<PhysicsMaterial> GetMaterial() const { return m_material; }

	private:
		friend class PhysicsMaterialSerializer;

		Ref<PhysicsMaterial> m_material;
	};
}
