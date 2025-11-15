#pragma once

#include <AssetSystem/AssetHandle.h>

#include <EntitySystem/EntityID.h>
#include <CoreUtilities/Archive/MemoryArchive.h>

namespace Volt::EntityDescSerialization
{
	struct MemberHeader
	{
		// #TODO_AssetSystem: Replace with uint identifier.
		std::string name;
		size_t size;
		size_t offset;

		VT_INLINE friend Archive& operator<<(Archive& archive, MemberHeader& value)
		{
			archive << value.name;
			archive << value.size;
			archive << value.offset;

			return archive;
		}
	};

	struct ComponentHeader
	{
		VoltGUID componentGUID;
		size_t componentDataOffset;

		VT_INLINE friend Archive& operator<<(Archive& archive, ComponentHeader& value)
		{
			archive << value.componentGUID;
			archive << value.componentDataOffset;

			return archive;
		}
	};

	struct SerializationData
	{
		EntityID entityId;
		AssetHandle ownerSceneAssetHandle;
		Vector<ComponentHeader, InlineAllocator<32u>> componentHeaders;
		MemoryReader componentData;
	};
}
