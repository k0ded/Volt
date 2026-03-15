#pragma once

#include "CoreUtilities/Config.h"
#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/Concepts.h"
#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Containers/Array.h"
#include "CoreUtilities/Containers/Map.h"
#include "CoreUtilities/VoltGUID.h"
#include "CoreUtilities/Buffer/DataBuffer.h"

#include <concepts>
#include <string>
#include <filesystem>

class Archive
{
public:
	VTCOREUTIL_API Archive(bool isLoading);
	virtual ~Archive() = default;

	VTCOREUTIL_API Archive(Archive&& other) noexcept;
	VTCOREUTIL_API Archive& operator=(Archive&& other) noexcept;

	VTCOREUTIL_API Archive(const Archive& other);
	VTCOREUTIL_API Archive& operator=(const Archive& other);

	virtual void SerializeBytes(void* value, size_t size) = 0;
	virtual void Reserve(size_t numBytes) = 0;
	virtual void Seek(size_t position) = 0;
	virtual void SetBasePosition(size_t position) = 0;
	virtual void Close() = 0;
	virtual void Serialize(Archive& arhive) {}
	virtual size_t GetHeadLocation() const = 0;
	virtual size_t GetSize() const = 0;
	virtual const void* GetData() const = 0;
	virtual void* GetData() = 0;
	virtual bool IsClosed() const = 0;

	VTCOREUTIL_API void UseVersion(const VoltGUID& guid);
	VTCOREUTIL_API int32_t GetVersion(const VoltGUID& guid) const;

	VT_NODISCARD VT_INLINE bool IsLoading() const { return m_isLoading; }

	// Common serialization operators
	VT_INLINE friend Archive& operator<<(Archive& archive, std::string& value)
	{
		size_t size = value.size();
		archive << size;

		if (archive.IsLoading())
		{
			value.resize(size);
		}

		if (size > 0)
		{
			archive.SerializeBytes(value.data(), value.size());
		}
		return archive;
	}

	VT_INLINE friend Archive& operator<<(Archive& archive, std::filesystem::path& value)
	{
		std::string tempString = value.string();
		archive << tempString;

		if (archive.IsLoading())
		{
			value = tempString;
		}

		return archive;
	}

	VT_INLINE friend Archive& operator<<(Archive& archive, VoltGUID& value)
	{
		archive << value.loPart;
		archive << value.hiPart;
		return archive;
	}

	template<typename T, typename Allocator>
	VT_INLINE friend Archive& operator<<(Archive& archive, Vector<T, Allocator>& value)
	{
		size_t size = value.size();
		archive << size;

		if (archive.IsLoading())
		{
			value.resize(size);
		}

		for (size_t i = 0; i < size; ++i)
		{
			archive << value[i];
		}
		return archive;
	}

	template<Pod T, typename Allocator>
	VT_INLINE friend Archive& operator<<(Archive& archive, Vector<T, Allocator>& value)
	{
		size_t size = value.size();
		archive << size;

		if (archive.IsLoading())
		{
			value.resize_uninitialized(size);
		}

		if (size > 0)
		{
			archive.SerializeBytes(value.data(), value.byte_size());
		}
		return archive;
	}

	template<typename T, size_t Count>
	VT_INLINE friend Archive& operator<<(Archive& archive, Array<T, Count>& value)
	{
		for (size_t i = 0; i < value.size(); ++i)
		{
			archive << value[i];
		}
		return archive;
	}

	template<Pod T, size_t Count>
	VT_INLINE friend Archive& operator<<(Archive& archive, Array<T, Count>& value)
	{
		archive.SerializeBytes(value.data(), value.byte_size());
		return archive;
	}

	template<typename Key, typename Value>
	friend Archive& operator<<(Archive& archive, Map<Key, Value>& map)
	{
		size_t size = map.size();
		archive << size;

		if (archive.IsLoading())
		{
			map.reserve(size);

			for (size_t i = 0; i < size; ++i)
			{
				Key k;
				Value v;

				archive << k;
				archive << v;

				map[k] = std::move(v);
			}
		}
		else
		{
			for (auto& [key, v] : map)
			{
				archive << key;
				archive << v;
			}
		}

		return archive;
	}

	VT_INLINE friend Archive& operator<<(Archive& archive, DataBuffer& buffer)
	{
		size_t size = buffer.GetSize();
		archive << size;

		if (archive.IsLoading())
		{
			buffer.Allocate(size);
		}

		if (size > 0)
		{
			archive.SerializeBytes(buffer.As<void*>(), size);
		}
		return archive;
	}

	VT_INLINE friend Archive& operator<<(Archive& archive, Archive& value)
	{
		if (&archive != &value)
		{
			value.Serialize(archive);
		}
		return archive;
	}

	template<Arithmetic T>
	VT_INLINE friend Archive& operator<<(Archive& archive, T& value)
	{
		archive.SerializeBytes(&value, sizeof(T));
		return archive;
	}

	template<MathType T>
	VT_INLINE friend Archive& operator<<(Archive& archive, T& value)
	{
		archive.SerializeBytes(&value, sizeof(T));
		return archive;
	}

	template<Enum T>
	VT_INLINE friend Archive& operator<<(Archive& archive, T& value)
	{ 
		using UnderlyingType = std::underlying_type_t<T>;
		UnderlyingType& tempValue = *reinterpret_cast<UnderlyingType*>(&value);
		archive << tempValue;
		return archive;
	}

protected:
	struct VersionInfo
	{
		VoltGUID guid;
		int32_t version;

		VT_INLINE friend Archive& operator<<(Archive& archive, VersionInfo& value)
		{
			archive << value.guid;
			archive << value.version;

			return archive;
		}
	};

	Vector<VersionInfo> m_versions;

private:
	const bool m_isLoading;
};
