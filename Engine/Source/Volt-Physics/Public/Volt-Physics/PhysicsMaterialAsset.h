#pragma once

#include "Volt-Physics/Config.h"

#include <PhysicsInterface/PhysicsMaterial.h>

#include <AssetSystem/Asset.h>

VT_DECLARE_ASSET_TYPE_EXPORT(PhysicsMaterial, "{78683B2B-E786-40FB-AEB3-82F9CA7E3E52}"_guid, VTP_API);

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
