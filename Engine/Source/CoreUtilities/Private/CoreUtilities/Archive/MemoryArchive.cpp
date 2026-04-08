#include "cupch.h"

#include "CoreUtilities/Archive/MemoryArchive.h"
#include "CoreUtilities/Archive/ArchiveVersionRegistry.h"

MemoryWriter::MemoryWriter()
	: Archive(false), m_isOpen(true)
{}

MemoryWriter::MemoryWriter(const MemoryWriter& other)
	: Archive(false),
	m_allocator(other.m_allocator),
	m_isOpen(other.m_isOpen)
{}

MemoryWriter& MemoryWriter::operator=(const MemoryWriter& other)
{
	if (this != &other)
	{
		m_isOpen = other.m_isOpen;
		m_allocator = other.m_allocator;
	}

	return *this;
}

void MemoryWriter::SerializeBytes(void* value, size_t size)
{
	size_t offset = m_allocator.size();
	m_allocator.resize_uninitialized(offset + size);

	memcpy_s(&m_allocator[offset], size, value, size);
}

void MemoryWriter::Reserve(size_t numBytes)
{
	m_allocator.reserve(numBytes);
}

void MemoryWriter::Seek(size_t position)
{
	VT_ENSURE(false);
}

void MemoryWriter::SetBasePosition(size_t position)
{
	VT_ENSURE(false);
}

void MemoryWriter::Close()
{
	SerializeVersions();

	m_isOpen = false;
}

void MemoryWriter::Serialize(Archive& archive)
{
	VT_ENSURE_MSG(!m_isOpen, "Archive must be closed to be serialized!");

	size_t size = GetSize();
	archive << size;

	if (size > 0)
	{
		archive.SerializeBytes(GetData(), size);
	}
}

size_t MemoryWriter::GetHeadLocation() const
{
	return m_allocator.size();
}

size_t MemoryWriter::GetSize() const
{
	return m_allocator.size();
}

const void* MemoryWriter::GetData() const
{
	VT_ENSURE_MSG(!m_isOpen, "Archive must be closed before it can be accessed!");
	return m_allocator.data();
}

void* MemoryWriter::GetData()
{
	VT_ENSURE_MSG(!m_isOpen, "Archive must be closed before it can be accessed!");
	return m_allocator.data();
}

bool MemoryWriter::IsClosed() const
{
	return !m_isOpen;
}

void MemoryWriter::SerializeVersions()
{
	// Fill the versions
	for (VersionInfo& versionInfo : m_versions)
	{
		versionInfo.version = ArchiveVersionRegistry::Get().TryGetVersion(versionInfo.guid);
	}

	// Serialize the version data to the archive.
	const size_t sizeBeforeVersionInfo = GetSize();
	(*this) << m_versions;

	const size_t sizeAfterVersionInfo = GetSize();
	const size_t versionInfoSize = sizeAfterVersionInfo - sizeBeforeVersionInfo;

	// Move the serialization data to the beginning of the archive.
	Vector<uint8_t> tempData;
	tempData.resize_uninitialized(versionInfoSize);

	memmove_s(tempData.data(), versionInfoSize, m_allocator.data() + sizeBeforeVersionInfo, versionInfoSize);
	memmove_s(m_allocator.data() + versionInfoSize, sizeBeforeVersionInfo, m_allocator.data(), sizeBeforeVersionInfo);
	memmove_s(m_allocator.data(), versionInfoSize, tempData.data(), versionInfoSize);
}

MemoryReader::MemoryReader()
	: Archive(true)
{

}

MemoryReader::MemoryReader(const void* srcData, size_t srcSize)
	: Archive(true)
{
	Parse(srcData, srcSize);
}

void MemoryReader::SerializeBytes(void* value, size_t size)
{
	VT_ENSURE(m_readPointer + size <= m_storage.size());

	memcpy_s(value, size, &m_storage[m_readPointer], size);
	m_readPointer += size;
}

void MemoryReader::Reserve(size_t numBytes)
{
	m_storage.resize_uninitialized(numBytes);
}

void MemoryReader::Seek(size_t position)
{
	VT_ENSURE(position < m_storage.size());
	m_readPointer = m_basePosition + position;
}

void MemoryReader::SetBasePosition(size_t position)
{
	m_basePosition = position;
	if (m_readPointer < m_basePosition)
	{
		m_readPointer = m_basePosition;
	}
}

void MemoryReader::Close()
{

}

void MemoryReader::Serialize(Archive& archive)
{
	size_t size;
	archive << size;

	m_storage.resize_uninitialized(size);

	if (size > 0)
	{
		archive.SerializeBytes(GetData(), size);
	}

	// Deserialize version info.
	(*this) << m_versions;

	// Make sure all Seek calls gets the correct positions.
	SetBasePosition(m_readPointer);
}

size_t MemoryReader::GetHeadLocation() const
{
	return m_readPointer;
}

size_t MemoryReader::GetSize() const
{
	return m_storage.size();
}

const void* MemoryReader::GetData() const
{
	return m_storage.data();
}

void* MemoryReader::GetData()
{
	return m_storage.data();
}

bool MemoryReader::IsClosed() const
{
	return true;
}

void MemoryReader::Parse(const void* srcData, size_t srcSize)
{
	m_storage.resize_uninitialized(srcSize);
	memcpy_s(m_storage.data(), srcSize, srcData, srcSize);

	// Deserialize version info.
	(*this) << m_versions;

	// Make sure all Seek calls gets the correct positions.
	SetBasePosition(m_readPointer);
}

VersionlessMemoryWriterExternal::VersionlessMemoryWriterExternal(Vector<uint8_t>& targetBytes)
	: Archive(false),
	m_bytes(targetBytes),
	m_isOpen(true)
{}

VersionlessMemoryWriterExternal::VersionlessMemoryWriterExternal(const VersionlessMemoryWriterExternal& other)
	: Archive(false),
	m_bytes(other.m_bytes),
	m_isOpen(other.m_isOpen)
{}

void VersionlessMemoryWriterExternal::SerializeBytes(void* value, size_t size)
{
	size_t offset = m_bytes.size();
	m_bytes.resize_uninitialized(offset + size);

	memcpy_s(&m_bytes[offset], size, value, size);
}

void VersionlessMemoryWriterExternal::Reserve(size_t numBytes)
{
	m_bytes.reserve(numBytes);
}

void VersionlessMemoryWriterExternal::Seek(size_t position)
{
	VT_ENSURE(false);
}

void VersionlessMemoryWriterExternal::SetBasePosition(size_t position)
{
	VT_ENSURE(false);
}

void VersionlessMemoryWriterExternal::Close()
{
	m_isOpen = false;
}

void VersionlessMemoryWriterExternal::Serialize(Archive& archive)
{
	VT_ENSURE_MSG(!m_isOpen, "Archive must be closed to be serialized!");

	size_t size = GetSize();
	archive << size;

	if (size > 0)
	{
		archive.SerializeBytes(GetData(), size);
	}
}

size_t VersionlessMemoryWriterExternal::GetHeadLocation() const
{
	return m_bytes.size();
}

size_t VersionlessMemoryWriterExternal::GetSize() const
{
	return m_bytes.size();
}

const void* VersionlessMemoryWriterExternal::GetData() const
{
	VT_ENSURE_MSG(!m_isOpen, "Archive must be closed before it can be accessed!");
	return m_bytes.data();
}

void* VersionlessMemoryWriterExternal::GetData()
{
	VT_ENSURE_MSG(!m_isOpen, "Archive must be closed before it can be accessed!");
	return m_bytes.data();
}

bool VersionlessMemoryWriterExternal::IsClosed() const
{
	return !m_isOpen;
}

VersionlessMemoryReaderExternal::VersionlessMemoryReaderExternal(Vector<uint8_t>& targetBytes)
	: Archive(true),
	m_bytes(targetBytes)
{}

VersionlessMemoryReaderExternal::VersionlessMemoryReaderExternal(const VersionlessMemoryReaderExternal& other)
	: Archive(true),
	m_bytes(other.m_bytes)
{}

void VersionlessMemoryReaderExternal::SerializeBytes(void* value, size_t size)
{
	VT_ENSURE(m_readPointer + size <= m_bytes.size());

	memcpy_s(value, size, &m_bytes[m_readPointer], size);
	m_readPointer += size;
}

void VersionlessMemoryReaderExternal::Reserve(size_t numBytes)
{
	m_bytes.resize_uninitialized(numBytes);
}

void VersionlessMemoryReaderExternal::Seek(size_t position)
{
	VT_ENSURE(position < m_bytes.size());
	m_readPointer = position;
}

void VersionlessMemoryReaderExternal::SetBasePosition(size_t position)
{
	VT_ENSURE(false);
}

void VersionlessMemoryReaderExternal::Close()
{}

void VersionlessMemoryReaderExternal::Serialize(Archive& archive)
{
	size_t size = m_bytes.size();
	archive << size;

	if (archive.IsLoading())
	{
		m_bytes.resize_uninitialized(size);
	}

	if (size > 0)
	{
		archive.SerializeBytes(GetData(), size);
	}
}

size_t VersionlessMemoryReaderExternal::GetHeadLocation() const
{
	return m_readPointer;
}

size_t VersionlessMemoryReaderExternal::GetSize() const
{
	return m_bytes.size();
}

const void* VersionlessMemoryReaderExternal::GetData() const
{
	return m_bytes.data();
}

void* VersionlessMemoryReaderExternal::GetData()
{
	return m_bytes.data();
}

bool VersionlessMemoryReaderExternal::IsClosed() const
{
	return true;
}

void VersionlessMemoryReaderExternal::Parse(const void* srcData, size_t srcSize)
{
	m_bytes.resize_uninitialized(srcSize);
	memcpy_s(m_bytes.data(), srcSize, srcData, srcSize);
}
