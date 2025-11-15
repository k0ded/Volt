#pragma once

#include "Volt-Scene/EntityDescSerializationCommon.h"
#include "Volt-Scene/Config.h"

#include <AssetSystem/AssetHandle.h>

#include <EntitySystem/Entity.h>
#include <CoreUtilities/Archive/MemoryArchive.h>

namespace Volt::EntityDescSerialization
{
	void SerializeEntity(Archive& archive, Entity entity, AssetHandle ownerSceneAssetHandle);
	void DeserializeEntityData(Archive& archive, SerializationData& outSerializationData);
	VTS_API void DeserializeEntity(Archive& archive, Entity entity);
	VTS_API void DeserializeEntity(Entity entity, SerializationData& serializationData);
}
