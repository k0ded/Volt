#pragma once

#include "CoreUtilities/Config.h"
#include "CoreUtilities/Archive/Archive.h"

class MemoryWriter : public Archive
{
public:
	VTCOREUTIL_API MemoryWriter();
	VTCOREUTIL_API MemoryWriter(const MemoryWriter& other);

	~MemoryWriter() override = default;

	VTCOREUTIL_API MemoryWriter& operator=(const MemoryWriter& other);

	VTCOREUTIL_API void SerializeBytes(void* value, size_t size) override;
	VTCOREUTIL_API void Reserve(size_t numBytes) override;
	VTCOREUTIL_API void Seek(size_t position) override;
	VTCOREUTIL_API void SetBasePosition(size_t position) override;
	VTCOREUTIL_API void Close() override;
	VTCOREUTIL_API void Serialize(Archive& archive) override;
	VTCOREUTIL_API size_t GetHeadLocation() const override;
	VTCOREUTIL_API size_t GetSize() const override;
	VTCOREUTIL_API const void* GetData() const override;
	VTCOREUTIL_API void* GetData() override;
	VTCOREUTIL_API bool IsClosed() const override;

private:
	void SerializeVersions();

	Vector<uint8_t> m_allocator;
	bool m_isOpen;
};

class MemoryReader : public Archive
{
public:
	VTCOREUTIL_API MemoryReader();
	VTCOREUTIL_API MemoryReader(const void* srcData, size_t srcSize);
	~MemoryReader() override = default;

	VTCOREUTIL_API void SerializeBytes(void* value, size_t size) override;
	VTCOREUTIL_API void Reserve(size_t numBytes) override;
	VTCOREUTIL_API void Seek(size_t position) override;
	VTCOREUTIL_API void SetBasePosition(size_t position) override;
	VTCOREUTIL_API void Close() override;
	VTCOREUTIL_API void Serialize(Archive& archive) override;
	VTCOREUTIL_API size_t GetHeadLocation() const override;
	VTCOREUTIL_API size_t GetSize() const override;
	VTCOREUTIL_API const void* GetData() const override;
	VTCOREUTIL_API void* GetData() override;
	VTCOREUTIL_API bool IsClosed() const override;

	VTCOREUTIL_API void Parse(const void* srcData, size_t srcSize);

private:
	Vector<uint8_t> m_storage;
	size_t m_readPointer = 0;
	size_t m_basePosition = 0;
};


class VersionlessMemoryWriterExternal : public Archive
{
public:
	VTCOREUTIL_API VersionlessMemoryWriterExternal(Vector<uint8_t>& targetBytes);
	VTCOREUTIL_API VersionlessMemoryWriterExternal(const VersionlessMemoryWriterExternal& other);

	VTCOREUTIL_API ~VersionlessMemoryWriterExternal() override = default;

	VTCOREUTIL_API void SerializeBytes(void* value, size_t size) override;
	VTCOREUTIL_API void Reserve(size_t numBytes) override;
	VTCOREUTIL_API void Seek(size_t position) override;
	VTCOREUTIL_API void SetBasePosition(size_t position) override;
	VTCOREUTIL_API void Close() override;
	VTCOREUTIL_API void Serialize(Archive& archive) override;
	VTCOREUTIL_API size_t GetHeadLocation() const override;
	VTCOREUTIL_API size_t GetSize() const override;
	VTCOREUTIL_API const void* GetData() const override;
	VTCOREUTIL_API void* GetData() override;
	VTCOREUTIL_API bool IsClosed() const override;

private:
	Vector<uint8_t>& m_bytes;
	bool m_isOpen;
};

class VersionlessMemoryReaderExternal : public Archive
{
public:
	VTCOREUTIL_API VersionlessMemoryReaderExternal(Vector<uint8_t>& targetBytes);
	VTCOREUTIL_API VersionlessMemoryReaderExternal(const VersionlessMemoryReaderExternal& other);

	VTCOREUTIL_API ~VersionlessMemoryReaderExternal() override = default;

	VTCOREUTIL_API void SerializeBytes(void* value, size_t size) override;
	VTCOREUTIL_API void Reserve(size_t numBytes) override;
	VTCOREUTIL_API void Seek(size_t position) override;
	VTCOREUTIL_API void SetBasePosition(size_t position) override;
	VTCOREUTIL_API void Close() override;
	VTCOREUTIL_API void Serialize(Archive& archive) override;
	VTCOREUTIL_API size_t GetHeadLocation() const override;
	VTCOREUTIL_API size_t GetSize() const override;
	VTCOREUTIL_API const void* GetData() const override;
	VTCOREUTIL_API void* GetData() override;
	VTCOREUTIL_API bool IsClosed() const override;

	VTCOREUTIL_API void Parse(const void* srcData, size_t srcSize);

private:
	Vector<uint8_t>& m_bytes;
	size_t m_readPointer = 0;
};
