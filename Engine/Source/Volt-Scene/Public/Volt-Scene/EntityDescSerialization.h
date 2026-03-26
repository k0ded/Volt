#pragma once

#include "Volt-Scene/Config.h"

#include "Volt-Scene/EntityDescSerializationCommon.h"

#include <EntitySystem/Entity.h>
#include <CoreUtilities/Containers/Vector.h>

namespace Volt::EntityDescSerialization
{
	VTS_API void SerializeEntityDescData(Archive& archive, SerializationData& entityData);

	VTS_API void GatherComponentData(Entity entity, ComponentData& outComponentData);
	VTS_API void UpdateSingleComponentInData(Entity entity, VoltGUID componentToUI, ComponentData& outComponentData);
	void UpdateHeaderByteRangeSize(ComponentData& componentData, ComponentHeader& header, size_t newSize);

	VTS_API void ApplyEntityDescData(Entity entity, SerializationData& serializationData);
	VTS_API void ApplyComponentData(Entity entity, ComponentData& componentData);

	// This should only be used for SAVING out entities. Please use DeserializeEntity in order to load entities
	// The reason for this is because the owning scene handle 
	VTS_API void SerializeEntity(Archive& archive, Entity entity, AssetHandle ownerSceneAssetHandle);
	VTS_API void DeserializeEntity(Archive& archive, Entity entity);


	VTS_API Filesystem::Path GetSavePathForEntity(const Volt::AssetHandle& handle);
}
