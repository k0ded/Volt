#pragma once

#include "Volt-Scene/EntityDescSerializationCommon.h"

#include <EntitySystem/Entity.h>
#include <CoreUtilities/Archive/MemoryArchive.h>

namespace Volt::EntityDescSerialization
{
	void SerializeEntity(Archive& archive, Entity entity);
	void DeserializeEntityData(Archive& archive, SerializationData& outSerializationData);
	void DeserializeEntity(Archive& archive, Entity entity);
}
