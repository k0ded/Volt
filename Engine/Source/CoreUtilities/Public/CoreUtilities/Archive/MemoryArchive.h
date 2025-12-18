#pragma once

#include "CoreUtilities/Archive/Archive.h"
#include "CoreUtilities/Config.h"

class MemoryWriter : public Archive
{
public:
	VTCOREUTIL_API MemoryWriter();
	~MemoryWriter() override = default;

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

private:
	Vector<uint8_t> m_storage;
	size_t m_readPointer = 0;
	size_t m_basePosition = 0;
};
