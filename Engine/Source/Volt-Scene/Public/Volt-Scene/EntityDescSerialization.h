#pragma once

#include "Volt-Scene/Config.h"

#include "Volt-Scene/EntityDescSerializationCommon.h"

#include <EntitySystem/Entity.h>
#include <CoreUtilities/Archive/MemoryArchive.h>

namespace Volt::EntityDescSerialization
{
	VTS_API void SerializeEntity(Archive& archive, Entity entity, AssetHandle ownerSceneAssetHandle);
	void DeserializeEntityData(Archive& archive, SerializationData& outSerializationData);
	VTS_API void DeserializeEntity(Archive& archive, Entity entity);
	VTS_API void DeserializeEntity(Entity entity, SerializationData& serializationData);
	VTS_API std::filesystem::path GetSavePathForEntity(const Volt::AssetHandle& handle);
}
