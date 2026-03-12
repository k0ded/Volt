#pragma once

#include "CoreUtilities/Archive/Archive.h"
#include "CoreUtilities/Config.h"

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
	VTCOREUTIL_API FileWriter();
	~FileWriter() override = default;

	VTCOREUTIL_API bool Open(const std::filesystem::path& destinationFilepath);
	VTCOREUTIL_API std::string_view GetError() const;

	VTCOREUTIL_API void SerializeBytes(void* value, size_t size) override;
	VTCOREUTIL_API void Reserve(size_t numBytes) override;
	VTCOREUTIL_API void Seek(size_t position) override;
	VTCOREUTIL_API void SetBasePosition(size_t position) override;
	VTCOREUTIL_API void Close() override;
	VTCOREUTIL_API size_t GetHeadLocation() const override;
	VTCOREUTIL_API size_t GetSize() const override;
	VTCOREUTIL_API const void* GetData() const override;
	VTCOREUTIL_API void* GetData() override;
	VTCOREUTIL_API bool IsClosed() const override;

private:
	void SerializeVersions();

	bool m_isOpen;
	std::string m_error;

	Vector<uint8_t> m_allocator;
	std::ofstream m_outputStream;
};

class FileReader : public Archive
{
public:
	VTCOREUTIL_API FileReader();
	~FileReader() override = default;

	VTCOREUTIL_API bool Open(const std::filesystem::path& filepath);
	VTCOREUTIL_API std::string_view GetError() const;

	VTCOREUTIL_API void SerializeBytes(void* value, size_t size) override;
	VTCOREUTIL_API void Reserve(size_t numBytes) override;
	VTCOREUTIL_API void Seek(size_t position) override;
	VTCOREUTIL_API void SetBasePosition(size_t position) override;
	VTCOREUTIL_API void Close() override;
	VTCOREUTIL_API size_t GetHeadLocation() const override;
	VTCOREUTIL_API size_t GetSize() const override;
	VTCOREUTIL_API const void* GetData() const override;
	VTCOREUTIL_API void* GetData() override;
	VTCOREUTIL_API bool IsClosed() const override;

private:
	std::ifstream m_inputStream;
	std::string m_error;
	bool m_isOpen;

	Vector<uint8_t> m_storage;
	size_t m_readPointer = 0;
	size_t m_basePosition = 0;
};
