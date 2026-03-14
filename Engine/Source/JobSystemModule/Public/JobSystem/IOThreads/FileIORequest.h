#pragma once

#include "JobSystem/IOThreads/IORequest.h"

#include <Volt-FileSystem/FileArchive.h>

namespace Volt
{
	class IORequestReadFile : public IORequest
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

		VTJS_API IORequestReadFile(std::string_view name, const std::filesystem::path& filepath, const Config& config = {});
		~IORequestReadFile() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		FileReader& GetResult() { return m_fileReader; }

	private:
		FileReader m_fileReader;
		Config m_config;
		std::filesystem::path m_filepath;
		IORequestResultCode m_resultCode;
	};

	class IORequestWriteFile : public IORequest
	{
	public:
		using ResultType = bool;

		VTJS_API IORequestWriteFile(std::string_view name, FileWriter&& fileWriter);
		~IORequestWriteFile() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		bool& GetResult() { return m_result; }

	private:
		FileWriter m_fileWriter;
		IORequestResultCode m_resultCode;
		bool m_result;
	};
}
