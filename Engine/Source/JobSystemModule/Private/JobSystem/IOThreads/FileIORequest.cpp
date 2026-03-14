#include "jspch.h"

#include "JobSystem/IOThreads/FileIORequest.h"

namespace Volt
{

	IORequestReadFile::IORequestReadFile(std::string_view name, const std::filesystem::path& filepath, const Config& config)
		: IORequest(name),
		m_config(config),
		m_filepath(filepath),
		m_resultCode(IORequestResultCode::Undefined)
	{
	}

	void IORequestReadFile::Execute()
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

	IORequestResultCode IORequestReadFile::GetResultCode() const
	{
		return m_resultCode;
	}

	IORequestWriteFile::IORequestWriteFile(std::string_view name, FileWriter&& fileWriter)
		: IORequest(name),
		m_fileWriter(std::move(fileWriter)),
		m_resultCode(IORequestResultCode::Undefined),
		m_result(false)
	{
		VT_ENSURE_MSG(!m_fileWriter.IsClosed(), "FileWriter must be open!");
	}

	void IORequestWriteFile::Execute()
	{
		m_fileWriter.Close();
		m_result = true;
		m_resultCode = IORequestResultCode::Success;
	}

	IORequestResultCode IORequestWriteFile::GetResultCode() const
	{
		return m_resultCode;
	}
}
