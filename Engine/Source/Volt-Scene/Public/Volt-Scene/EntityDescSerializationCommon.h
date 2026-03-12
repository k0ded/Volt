#pragma once

#include <AssetSystem/AssetHandle.h>

#include <EntitySystem/EntityID.h>
#include <CoreUtilities/Archive/MemoryArchive.h>

namespace Volt::EntityDescSerialization
{
	constexpr uint32_t MAX_COMPONENT_COUNT = 32u;

	struct EntityDescArchiveVersion
	{
		enum Type
		{
			BaseVersion = 0,

			// added serialization of component data size
			// change componentdata from being an archive to an array of bytes instead
			AllowModifyingPartsOfSavedata = 1,

			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		inline static constexpr VoltGUID guid = "{6FE8424E-8B86-412C-B793-6CBD6EB919AE}"_guid;

	private:
		EntityDescArchiveVersion() {}
	};

	struct MemberHeader
	{
		uint32_t identifier;
		size_t size;
		size_t offset;

		VT_INLINE friend Archive& operator<<(Archive& archive, MemberHeader& value)
		{
			archive << value.identifier;
			archive << value.size;
			archive << value.offset;

			return archive;
		}
	};

	struct ComponentHeader
	{
		VoltGUID componentGUID;
		size_t componentDataOffset;
		size_t componentDataSize;

		VT_INLINE friend Archive& operator<<(Archive& archive, ComponentHeader& value)
		{
			const int32_t entityDescVersion = archive.GetVersion(EntityDescArchiveVersion::guid);

			archive << value.componentGUID;
			archive << value.componentDataOffset;

			if (archive.IsLoading())
			{
				if (entityDescVersion >= EntityDescArchiveVersion::AllowModifyingPartsOfSavedata)
				{
					archive << value.componentDataSize;
				}
				else
				{
					value.componentDataSize = 0;
				}
			}
			else
			{
				archive << value.componentDataSize;
			}
			

			return archive;
		}
	};

	struct ComponentData
	{
		Vector<ComponentHeader, InlineAllocator<MAX_COMPONENT_COUNT>> headers;
		Vector<uint8_t> data;

		VT_INLINE friend Archive& operator<<(Archive& archive, ComponentData& value)
		{
			const int32_t entityDescVersion = archive.GetVersion(EntityDescArchiveVersion::guid);


			archive << value.headers;

			if (archive.IsLoading())
			{
				if (entityDescVersion >= EntityDescArchiveVersion::AllowModifyingPartsOfSavedata)
				{
					archive << value.data;
				}
				else
				{
					MemoryReader reader;
					archive << reader;
					//the archive originally had no version, so we need to remove the 8 bytes that correspond to the versions count (size_t) in the beginning
					const size_t byteCount = reader.GetSize() - 8;
					value.data.resize_uninitialized(byteCount);
					memcpy_s(value.data.data(), value.data.size(), reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(reader.GetData()) + 8), byteCount);
				}
			}
			else
			{
				archive << value.data;
			}
			

			return archive;
		}
	};

	struct SerializationData
	{
		EntityID entityId;
		AssetHandle ownerSceneAssetHandle;
		ComponentData components;
	};
}
