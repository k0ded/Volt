#include "Volt-FileSystem/FileArchive.h"

#include <Volt-Platforms/FileHandle.h>
#include <Volt-Platforms/Platform.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/Archive/ArchiveVersionRegistry.h>
#include <CoreUtilities/Profiling/Profiling.h>

FileWriter::FileWriter()
	: Archive(false),
	m_isOpen(false)
{

}

FileWriter::FileWriter(FileWriter&& other) noexcept
	: Archive(std::move(other)),
	m_isOpen(other.m_isOpen),
	m_fileHandle(std::move(other.m_fileHandle)),
	m_error(std::move(other.m_error)),
	m_allocator(std::move(other.m_allocator))
{
	other.m_isOpen = false;
}

FileWriter& FileWriter::operator=(FileWriter&& other) noexcept
{
	if (&other != this)
	{
		m_isOpen = other.m_isOpen;
		m_fileHandle = std::move(other.m_fileHandle);
		m_error = std::move(other.m_error);
		m_allocator = std::move(other.m_allocator);
	
		other.m_isOpen = false;
	}

	return *this;
}

FileWriter::~FileWriter()
{
	if (m_fileHandle.IsValid())
	{
		Volt::PlatformFileSystem::CloseFile(m_fileHandle);
	}
}

bool FileWriter::Open(const std::filesystem::path& destinationFilepath)
{
	VT_PROFILE_FUNCTION();

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

	m_fileHandle = Volt::PlatformFileSystem::CreateFile(destinationFilepath);
	m_isOpen = m_fileHandle.IsValid();

	if (!m_fileHandle.IsValid())
	{
		m_error = std::format("I/O error while writing '{}'", destinationFilepath.string());
		return false;
	}

	return m_fileHandle.IsValid();
}

std::string_view FileWriter::GetError() const
{
	return m_error;
}

void FileWriter::SerializeBytes(void* value, size_t size)
{
	VT_ENSURE_MSG(m_isOpen, "Writer must be open to be writeable!");

	// No need to serialize nothing.
	if (size == 0)
	{
		return;
	}

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
	VT_PROFILE_FUNCTION();

	VT_ENSURE_MSG(m_fileHandle.IsValid(), "No file is open!");

	if (!VT_CHECK_MSG(Volt::PlatformThread::GetThreadConfig().isIOThread, "FileReader::Open may only be called on an IO thread!"))
	{
		return;
	}

	SerializeVersions();

	FileArchiveHeader header;
	header.magic = FileArchiveHeader::MagicValue;
	header.isCompressed = false;
	header.compressedSize = 0;

	Volt::PlatformFileSystem::WriteFile(m_fileHandle, &header, sizeof(FileArchiveHeader));
	Volt::PlatformFileSystem::WriteFile(m_fileHandle, m_allocator.data(), m_allocator.size());
	Volt::PlatformFileSystem::CloseFile(m_fileHandle);

	m_fileHandle.Reset();
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

bool FileWriter::IsClosed() const
{
	return !m_isOpen;
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

bool FileReader::Open(const std::filesystem::path& filepath, const FileReaderConfig& config)
{
	VT_PROFILE_FUNCTION();

	if (!VT_CHECK_MSG(Volt::PlatformThread::GetThreadConfig().isIOThread, "FileReader::Open may only be called on an IO thread!"))
	{
		return false;
	}

	const bool isSmallRead = config.maxReadSize > 0 && config.maxReadSize < 4096;

	Volt::FileHandle fileHandle = Volt::PlatformFileSystem::OpenFile(filepath, false, isSmallRead);
	m_isOpen = fileHandle.IsValid();

	if (m_isOpen)
	{
		uint64_t fileSize = Volt::PlatformFileSystem::GetFileSize(fileHandle);
		fileSize -= sizeof(FileArchiveHeader);
		fileSize = config.maxReadSize > 0 ? std::min(fileSize, config.maxReadSize) : fileSize;

		FileArchiveHeader fileArchiveHeader;
		Volt::PlatformFileSystem::ReadFile(fileHandle, sizeof(FileArchiveHeader), &fileArchiveHeader, sizeof(FileArchiveHeader));

		// Make sure this is a file written by the file writer.
		if (fileArchiveHeader.magic != FileArchiveHeader::MagicValue)
		{
			m_isOpen = false;
			m_error = std::format("File '{}' was not written with a file archive!", filepath.string());
			return false;
		}

		m_storage.resize_uninitialized(fileSize);
		Volt::PlatformFileSystem::ReadFile(fileHandle, fileSize, m_storage.data(), m_storage.size());
		Volt::PlatformFileSystem::CloseFile(fileHandle);

		// Deserialize version info.
		(*this) << m_versions;

		// Make sure all Seek calls gets the correct positions.
		SetBasePosition(m_readPointer);
	}
	else
	{
		m_error = std::format("Failed to open file '{}'", filepath.string());
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
	VT_ENSURE(m_basePosition + position < m_storage.size());
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
	VT_ENSURE_MSG(!m_isOpen, "Archive must be closed before it can be accessed!");
	return m_storage.data();
}

void* FileReader::GetData()
{
	VT_ENSURE_MSG(!m_isOpen, "Archive must be closed before it can be accessed!");
	return m_storage.data();
}

bool FileReader::IsClosed() const
{
	return !m_isOpen;
}
