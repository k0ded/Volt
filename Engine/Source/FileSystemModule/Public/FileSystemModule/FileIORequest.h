#pragma once

#include "FileSystemModule/IOThreads/IORequest.h"
#include "FileSystemModule/FileArchive.h"

namespace Volt
{
	class IORequestReadFile_FileReader : public IORequest
	{
	public:
		using ResultType = FileReader;

		struct Config
		{
			/*
				Max bytes to read from file, 0 will read the entire file.
			*/
			uint64_t maxReadSize = 0;
		};

		VTFS_API IORequestReadFile_FileReader(StringView name, const Filesystem::Path& filepath, const Config& config = {});
		~IORequestReadFile_FileReader() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		FileReader& GetResult() { return m_fileReader; }

	private:
		FileReader m_fileReader;
		Config m_config;
		Filesystem::Path m_filepath;
		IORequestResultCode m_resultCode;
	};

	class IORequestReadFile_String : public IORequest
	{
	public:
		using ResultType = String;

		VTFS_API IORequestReadFile_String(StringView name, const Filesystem::Path& filepath);
		~IORequestReadFile_String() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		String& GetResult() { return m_result; }

	private:
		String m_result;
		Filesystem::Path m_filepath;
		IORequestResultCode m_resultCode;
	};

	class IORequestWriteFile_FileWriter : public IORequest
	{
	public:
		using ResultType = bool;

		VTFS_API IORequestWriteFile_FileWriter(StringView name, FileWriter&& fileWriter);
		~IORequestWriteFile_FileWriter() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		bool& GetResult() { return m_result; }

	private:
		FileWriter m_fileWriter;
		IORequestResultCode m_resultCode;
		bool m_result;
	};

	class IORequestWriteFile_String : public IORequest
	{
	public:
		using ResultType = bool;

		VTFS_API IORequestWriteFile_String(StringView name, const Filesystem::Path& filepath, String&& string);
		~IORequestWriteFile_String() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		bool& GetResult() { return m_result; }

	private:
		Filesystem::Path m_filepath;
		String m_string;
		IORequestResultCode m_resultCode;
		bool m_result;
	};
}
