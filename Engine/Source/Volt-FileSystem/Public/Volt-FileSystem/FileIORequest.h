#pragma once

#include "Volt-FileSystem/IOThreads/IORequest.h"
#include "Volt-FileSystem/FileArchive.h"

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

		VTFS_API IORequestReadFile_FileReader(std::string_view name, const std::filesystem::path& filepath, const Config& config = {});
		~IORequestReadFile_FileReader() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		FileReader& GetResult() { return m_fileReader; }

	private:
		FileReader m_fileReader;
		Config m_config;
		std::filesystem::path m_filepath;
		IORequestResultCode m_resultCode;
	};

	class IORequestReadFile_String : public IORequest
	{
	public:
		using ResultType = std::string;

		VTFS_API IORequestReadFile_String(std::string_view name, const std::filesystem::path& filepath);
		~IORequestReadFile_String() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		std::string& GetResult() { return m_result; }

	private:
		std::string m_result;
		std::filesystem::path m_filepath;
		IORequestResultCode m_resultCode;
	};

	class IORequestWriteFile_FileWriter : public IORequest
	{
	public:
		using ResultType = bool;

		VTFS_API IORequestWriteFile_FileWriter(std::string_view name, FileWriter&& fileWriter);
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

		VTFS_API IORequestWriteFile_String(std::string_view name, const std::filesystem::path& filepath, std::string&& string);
		~IORequestWriteFile_String() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		bool& GetResult() { return m_result; }

	private:
		std::filesystem::path m_filepath;
		std::string m_string;
		IORequestResultCode m_resultCode;
		bool m_result;
	};
}
