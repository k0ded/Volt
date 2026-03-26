#pragma once

#include "CoreUtilities/Config.h"
#include "CoreUtilities/CompilerTraits.h"
#include "CoreUtilities/Concepts.h"
#include "CoreUtilities/Containers/Vector.h"
#include "CoreUtilities/Containers/Array.h"
#include "CoreUtilities/Containers/Map.h"
#include "CoreUtilities/VoltGUID.h"
#include "CoreUtilities/UUID.h"
#include "CoreUtilities/Filesystem/Path.h"

#include <concepts>

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

protected:
	struct VersionInfo
	{
		VoltGUID guid;
		int32_t version;
	};

	VTCOREUTIL_API friend Archive& operator<<(Archive& archive, VersionInfo& value);

	Vector<VersionInfo> m_versions;

private:
	const bool m_isLoading;
};

// Common serialization operators
VTCOREUTIL_API Archive& operator<<(Archive& archive, String& value);
VTCOREUTIL_API Archive& operator<<(Archive& archive, Filesystem::Path& value);
VTCOREUTIL_API Archive& operator<<(Archive& archive, VoltGUID& value);
VTCOREUTIL_API Archive& operator<<(Archive& archive, Archive& value);
VTCOREUTIL_API Archive& operator<<(Archive& archive, UUID64& value);
VTCOREUTIL_API Archive& operator<<(Archive& archive, UUID32& value);

template<typename T, typename Allocator>
Archive& operator<<(Archive& archive, Vector<T, Allocator>& value);

template<Pod T, typename Allocator>
Archive& operator<<(Archive& archive, Vector<T, Allocator>& value);

template<typename T, size_t Count>
Archive& operator<<(Archive& archive, Array<T, Count>& value);

template<Pod T, size_t Count>
Archive& operator<<(Archive& archive, Array<T, Count>& value);

template<typename Key, typename Value>
Archive& operator<<(Archive& archive, Map<Key, Value>& map);

template<Arithmetic T>
Archive& operator<<(Archive& archive, T& value);

template<MathType T>
Archive& operator<<(Archive& archive, T& value);

template<Enum T>
Archive& operator<<(Archive& archive, T& value);

template<typename T, typename V>
Archive& operator<<(Archive& archive, std::pair<T, V>& value);

#include "CoreUtilities/Archive/Archive.inl"
