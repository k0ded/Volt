#pragma once

#include "Volt-FileSystem/Config.h"

#include <Volt-Platforms/FileHandle.h>

#include <CoreUtilities/Archive/Archive.h>

#include <fstream>

struct FileArchiveHeader
{
	inline static constexpr uint32_t MagicValue = 16789;

	uint32_t magic;
	uint32_t compressedSize;
	bool isCompressed;
};

class FileWriter : public Archive
{
public:
	VTFS_API FileWriter();
	VTFS_API ~FileWriter() override;

	VTFS_API FileWriter(FileWriter&& other) noexcept;
	VTFS_API FileWriter& operator=(FileWriter&& other) noexcept;

	VTFS_API bool Open(const Filesystem::Path& destinationFilepath);
	VTFS_API StringView GetError() const;

	VTFS_API void SerializeBytes(void* value, size_t size) override;
	VTFS_API void Reserve(size_t numBytes) override;
	VTFS_API void Seek(size_t position) override;
	VTFS_API void SetBasePosition(size_t position) override;
	VTFS_API void Close() override;
	VTFS_API size_t GetHeadLocation() const override;
	VTFS_API size_t GetSize() const override;
	VTFS_API const void* GetData() const override;
	VTFS_API void* GetData() override;
	VTFS_API bool IsClosed() const override;

private:
	void SerializeVersions();

	bool m_isOpen;
	Volt::FileHandle m_fileHandle;
	String m_error;
	Vector<uint8_t> m_allocator;
};

struct FileReaderConfig
{
	/*
		Max bytes to read from file, 0 will read the entire file.
	*/
	uint64_t maxReadSize = 0;
};

class FileReader : public Archive
{
public:
	VTFS_API FileReader();
	~FileReader() override = default;

	VTFS_API bool Open(const Filesystem::Path& filepath, const FileReaderConfig& config = {});
	VTFS_API StringView GetError() const;

	VTFS_API void SerializeBytes(void* value, size_t size) override;
	VTFS_API void Reserve(size_t numBytes) override;
	VTFS_API void Seek(size_t position) override;
	VTFS_API void SetBasePosition(size_t position) override;
	VTFS_API void Close() override;
	VTFS_API size_t GetHeadLocation() const override;
	VTFS_API size_t GetSize() const override;
	VTFS_API const void* GetData() const override;
	VTFS_API void* GetData() override;
	VTFS_API bool IsClosed() const override;

private:
	String m_error;
	bool m_isOpen;

	Vector<uint8_t> m_storage;
	size_t m_readPointer = 0;
	size_t m_basePosition = 0;
};
