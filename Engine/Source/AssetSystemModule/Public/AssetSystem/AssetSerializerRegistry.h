#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetType.h"
#include "AssetSystem/Serialization/AssetSerializer.h"

#include <unordered_map>

class VTAS_API AssetSerializerRegistry
{
public:
	bool RegisterAssetSerializer(VoltGUID typeGuid, Ref<Volt::AssetSerializer> serializer);

	Volt::AssetSerializer& GetSerializer(AssetType type) const;
	VT_INLINE bool HasSerializer(AssetType type) const { return m_serializers.contains(type->GetGUID()); }

	static AssetSerializerRegistry& Get();

private:
	Map<VoltGUID, Ref<Volt::AssetSerializer>> m_serializers;
};

#define VT_REGISTER_ASSET_SERIALIZER(type, serializer) \
	inline static bool AssetSerializer_ ## serializer ## _Registered = AssetSerializerRegistry::Get().RegisterAssetSerializer(type ## Type::guid, CreateRef<serializer>())
