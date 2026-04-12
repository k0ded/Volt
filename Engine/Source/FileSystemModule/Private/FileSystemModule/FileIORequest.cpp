#include "FileSystemModule/FileIORequest.h"

#include <PlatformsModule/Platform.h>

namespace Volt
{
	IORequestReadFile_FileReader::IORequestReadFile_FileReader(StringView name, const Filesystem::Path& filepath, const Config& config)
		: IORequest(name),
		m_config(config),
		m_filepath(filepath),
		m_resultCode(IORequestResultCode::Undefined)
	{
	}

	void IORequestReadFile_FileReader::Execute()
	{
		FileReaderConfig fileReaderConfig;
		fileReaderConfig.maxReadSize = m_config.maxReadSize;

		bool isOpen = m_fileReader.Open(m_filepath, fileReaderConfig);
		if (isOpen)
		{
			m_resultCode = IORequestResultCode::Success;
		}
		else
		{
			m_resultCode = IORequestResultCode::Failure;
		}
	}

	IORequestResultCode IORequestReadFile_FileReader::GetResultCode() const
	{
		return m_resultCode;
	}

	IORequestWriteFile_FileWriter::IORequestWriteFile_FileWriter(StringView name, FileWriter&& fileWriter)
		: IORequest(name),
		m_fileWriter(std::move(fileWriter)),
		m_resultCode(IORequestResultCode::Undefined)
	{
		VT_ENSURE_MSG(!m_fileWriter.IsClosed(), "FileWriter must be open!");
	}

	void IORequestWriteFile_FileWriter::Execute()
	{
		m_fileWriter.Close();
		m_resultCode = IORequestResultCode::Success;
	}

	IORequestResultCode IORequestWriteFile_FileWriter::GetResultCode() const
	{
		return m_resultCode;
	}

	IORequestReadFile_String::IORequestReadFile_String(StringView name, const Filesystem::Path& filepath)
		: IORequest(name),
		m_filepath(filepath),
		m_resultCode(IORequestResultCode::Undefined)
	{}
	
	void IORequestReadFile_String::Execute()
	{
		FileHandle fileHandle = PlatformFileSystem::OpenFile(m_filepath, false, false);

		if (fileHandle.IsValid())
		{
			const uint64_t fileSize = PlatformFileSystem::GetFileSize(fileHandle);
			m_result.resize(fileSize);

			PlatformFileSystem::ReadFile(fileHandle, fileSize, m_result.data(), m_result.size());
			PlatformFileSystem::CloseFile(fileHandle);

			m_resultCode = IORequestResultCode::Success;
		}
		else
		{
			m_resultCode = IORequestResultCode::Failure;
		}
	}

	IORequestResultCode IORequestReadFile_String::GetResultCode() const
	{
		return m_resultCode;
	}

	IORequestWriteFile_String::IORequestWriteFile_String(StringView name, const Filesystem::Path& filepath, String&& string)
		: IORequest(name),
		m_filepath(filepath),
		m_string(std::move(string)),
		m_resultCode(IORequestResultCode::Undefined)
	{
	}

	IORequestWriteFile_String::IORequestWriteFile_String(StringView name, const Filesystem::Path& filepath, StringView string)
		: IORequest(name),
		m_filepath(filepath),
		m_string(string),
		m_resultCode(IORequestResultCode::Undefined)
	{}

	void IORequestWriteFile_String::Execute()
	{
		FileHandle fileHandle = PlatformFileSystem::CreateFile(m_filepath);

		if (!fileHandle.IsValid())
		{
			m_resultCode = IORequestResultCode::Failure;
			return;
		}

		PlatformFileSystem::WriteFile(fileHandle, m_string.data(), m_string.size());
		PlatformFileSystem::CloseFile(fileHandle);

		m_resultCode = IORequestResultCode::Success;
	}

	IORequestResultCode IORequestWriteFile_String::GetResultCode() const
	{
		return m_resultCode;
	}
	
	IORequestReadFile_Binary::IORequestReadFile_Binary(StringView name, const Filesystem::Path& filepath)
		: IORequest(name),
		m_filepath(filepath),
		m_resultCode(IORequestResultCode::Undefined)
	{}
	
	void IORequestReadFile_Binary::Execute()
	{
		FileHandle fileHandle = PlatformFileSystem::OpenFile(m_filepath, false, false);

		if (fileHandle.IsValid())
		{
			const uint64_t fileSize = PlatformFileSystem::GetFileSize(fileHandle);
			m_result.resize_uninitialized(fileSize);

			PlatformFileSystem::ReadFile(fileHandle, fileSize, m_result.data(), m_result.size());
			PlatformFileSystem::CloseFile(fileHandle);

			m_resultCode = IORequestResultCode::Success;
		}
		else
		{
			m_resultCode = IORequestResultCode::Failure;
		}
	}
	
	IORequestResultCode IORequestReadFile_Binary::GetResultCode() const
	{
		return m_resultCode;
	}

	IORequestWriteFile_Binary::IORequestWriteFile_Binary(StringView name, const Filesystem::Path& filepath, const void* data, uint64_t dataSize, bool createCopyOfData)
		: IORequest(name),
		m_filepath(filepath),
		m_data(data),
		m_dataSize(dataSize),
		m_resultCode(IORequestResultCode::Undefined),
		m_createCopy(createCopyOfData)
	{
		if (createCopyOfData)
		{
			m_data = Memory::Malloc(dataSize);
			memcpy_s(const_cast<void*>(m_data), m_dataSize, data, dataSize);
		}
	}
	
	IORequestWriteFile_Binary::~IORequestWriteFile_Binary()
	{
		if (m_createCopy)
		{
			Memory::Free(const_cast<void*>(m_data));
		}
	}
	
	void IORequestWriteFile_Binary::Execute()
	{
		FileHandle fileHandle = PlatformFileSystem::CreateFile(m_filepath);

		if (!fileHandle.IsValid())
		{
			m_resultCode = IORequestResultCode::Failure;
			return;
		}

		PlatformFileSystem::WriteFile(fileHandle, m_data, m_dataSize);
		PlatformFileSystem::CloseFile(fileHandle);

		m_resultCode = IORequestResultCode::Success;
	}
	
	IORequestResultCode IORequestWriteFile_Binary::GetResultCode() const
	{
		return m_resultCode;
	}
}
