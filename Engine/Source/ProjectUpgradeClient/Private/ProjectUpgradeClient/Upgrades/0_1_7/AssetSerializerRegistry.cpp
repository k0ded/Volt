#include "AssetSerializerRegistry.h"

AssetSerializerRegistry g_assetSerializerRegistry;

bool AssetSerializerRegistry::RegisterAssetSerializer(VoltGUID typeGuid, Ref<Volt::AssetSerializer> serializer)
{
	VT_ASSERT(!m_serializers.contains(typeGuid));
	m_serializers[typeGuid] = std::move(serializer);
	return true;
}

Volt::AssetSerializer& AssetSerializerRegistry::GetSerializer(AssetType type) const
{
	VT_ENSURE(m_serializers.contains(type->GetGUID()));
	return *m_serializers.at(type->GetGUID());
}

AssetSerializerRegistry& AssetSerializerRegistry::Get()
{
	return g_assetSerializerRegistry;
}
