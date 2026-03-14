#include "jspch.h"

#include "JobSystem/IOThreads/FileIORequest.h"

namespace Volt
{

	IORequestReadFile::IORequestReadFile(std::string_view name, const std::filesystem::path& filepath)
		: IORequest(name),
		m_filepath(filepath),
		m_resultCode(IORequestResultCode::Undefined)
	{
	}

	void IORequestReadFile::Execute()
	{
		bool isOpen = m_fileReader.Open(m_filepath);
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
}
