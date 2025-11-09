#include "cupch.h"

#include "CoreUtilities/Archive/FileArchive.h"
#include "CoreUtilities/FileSystem.h"
#include "CoreUtilities/Archive/ArchiveVersionRegistry.h"

FileWriter::FileWriter()
	: Archive(false),
	m_isOpen(false)
{

}

bool FileWriter::Open(const std::filesystem::path& destinationFilepath)
{
	if (FileSystem::Exists(destinationFilepath) && !FileSystem::IsWriteable(destinationFilepath))
	{
		return false;
	}

	// Create the required directory tree
	if (!FileSystem::Exists(destinationFilepath.parent_path()))
	{
		FileSystem::CreateDirectories(destinationFilepath.parent_path());
	}

	m_outputStream.open(destinationFilepath, std::ios::out | std::ios::trunc | std::ios::binary);
	m_isOpen = m_outputStream.is_open();

	return m_outputStream.is_open();
}

void FileWriter::SerializeBytes(void* value, size_t size)
{
	VT_ENSURE_MSG(m_isOpen, "Writer must be open to be writeable!");

	size_t offset = m_allocator.size();
	m_allocator.resize_uninitialized(offset + size);

	memcpy_s(&m_allocator[offset], size, value, size);
}

void FileWriter::Reserve(size_t numBytes)
{
	m_allocator.reserve(numBytes);
}

void FileWriter::Seek(size_t position)
{
	VT_ENSURE(false);
}

void FileWriter::Close()
{
	SerializeVersions();

	m_outputStream.write(reinterpret_cast<const char*>(m_allocator.data()), m_allocator.size());
	m_outputStream.close();

	m_isOpen = false;
}

size_t FileWriter::GetHeadLocation() const
{
	return m_allocator.size();
}

size_t FileWriter::GetSize() const
{
	return m_allocator.size();
}

const void* FileWriter::GetData() const
{
	VT_ENSURE_MSG(!m_isOpen, "Archive must be closed before it can be accessed!");
	return m_allocator.data();
}

void* FileWriter::GetData()
{
	VT_ENSURE_MSG(!m_isOpen, "Archive must be closed before it can be accessed!");
	return m_allocator.data();
}

void FileWriter::SerializeVersions()
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

FileReader::FileReader()
	: Archive(true),
	m_isOpen(false)
{

}

bool FileReader::Open(const std::filesystem::path& filepath)
{
	m_inputStream.open(filepath, std::ios::in | std::ios::binary | std::ios::ate);
	m_isOpen = m_inputStream.is_open();

	if (m_inputStream.is_open())
	{
		size_t size = m_inputStream.tellg();
		m_inputStream.seekg(0);

		m_storage.resize_uninitialized(size);
		m_inputStream.read(reinterpret_cast<char*>(m_storage.data()), size);
		m_inputStream.close();

		// Deserialize version info.
		(*this) << m_versions;
	}

	return m_isOpen;
}

void FileReader::SerializeBytes(void* value, size_t size)
{
	VT_ENSURE(m_readPointer + size <= m_storage.size());

	memcpy_s(value, size, &m_storage[m_readPointer], size);
	m_readPointer += size;
}

void FileReader::Reserve(size_t numBytes)
{
	m_storage.resize_uninitialized(numBytes);
}

void FileReader::Seek(size_t position)
{
	VT_ENSURE(position < m_storage.size());
	m_readPointer = position;
}

void FileReader::Close()
{
	m_isOpen = false;
}

size_t FileReader::GetHeadLocation() const
{
	return m_readPointer;
}

size_t FileReader::GetSize() const
{
	return m_storage.size();
}

const void* FileReader::GetData() const
{
	return m_storage.data();
}

void* FileReader::GetData()
{
	return m_storage.data();
}
