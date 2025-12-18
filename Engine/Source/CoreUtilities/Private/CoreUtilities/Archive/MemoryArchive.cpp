#include "cupch.h"

#include "CoreUtilities/Archive/MemoryArchive.h"
#include "CoreUtilities/Archive/ArchiveVersionRegistry.h"

MemoryWriter::MemoryWriter()
	: Archive(false), m_isOpen(true)
{}

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
	m_storage.resize_uninitialized(srcSize);
	memcpy_s(m_storage.data(), srcSize, srcData, srcSize);

	// Deserialize version info.
	(*this) << m_versions;

	// Make sure all Seek calls gets the correct positions.
	SetBasePosition(m_readPointer);
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
