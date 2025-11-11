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
		m_error = std::format("Filepath '{}' is not writeable!", destinationFilepath.string());
		return false;
	}

	// Create the required directory tree
	if (!FileSystem::Exists(destinationFilepath.parent_path()))
	{
		FileSystem::CreateDirectories(destinationFilepath.parent_path());
	}

	m_outputStream.open(destinationFilepath, std::ios::out | std::ios::trunc | std::ios::binary);
	m_isOpen = m_outputStream.is_open();

	if (m_outputStream.bad())
	{
		m_error = std::format("I/O error while writing '{}'", destinationFilepath.string());
		return false;
	}

	return m_outputStream.is_open();
}

std::string_view FileWriter::GetError() const
{
	return m_error;
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

void FileWriter::SetBasePosition(size_t position)
{
	VT_ENSURE(false);
}

void FileWriter::Close()
{
	SerializeVersions();

	FileArchiveHeader header;
	header.magic = FileArchiveHeader::MagicValue;
	header.isCompressed = false;
	header.compressedSize = 0;

	m_outputStream.write(reinterpret_cast<const char*>(&header), sizeof(FileArchiveHeader));
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

	if (m_isOpen)
	{
		size_t size = m_inputStream.tellg();
		size -= sizeof(FileArchiveHeader);
		m_inputStream.seekg(0);

		FileArchiveHeader fileWriterHeader;
		m_inputStream.read(reinterpret_cast<char*>(&fileWriterHeader), sizeof(FileArchiveHeader));

		// Make sure this is a file written by the file writer.
		if (fileWriterHeader.magic != FileArchiveHeader::MagicValue)
		{
			m_isOpen = false;
			m_error = std::format("File '{}' was not written with a file archive!", filepath.string());
			return false;
		}

		m_storage.resize_uninitialized(size);
		m_inputStream.read(reinterpret_cast<char*>(m_storage.data()), size);
		m_inputStream.close();

		// Deserialize version info.
		(*this) << m_versions;

		// Make sure all Seek calls gets the correct positions.
		SetBasePosition(m_readPointer);
	}
	else
	{
		if (m_inputStream.bad())
		{
			m_error = std::format("I/O error while reading '{}'", filepath.string());
		}
		else
		{
			m_error = std::format("Failed to open file '{}'", filepath.string());
		}
	}

	return m_isOpen;
}

std::string_view FileReader::GetError() const
{
	return m_error;
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
	m_readPointer = m_basePosition + position;
}

void FileReader::SetBasePosition(size_t position)
{
	m_basePosition = position;
	if (m_readPointer < m_basePosition)
	{
		m_readPointer = m_basePosition;
	}
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
