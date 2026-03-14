#pragma once

#include "JobSystem/IOThreads/IORequest.h"

#include <CoreUtilities/Archive/FileArchive.h>

namespace Volt
{
	class IORequestReadFile : public IORequest
	{
	public:
		using ResultType = FileReader;

		VTJS_API IORequestReadFile(std::string_view name, const std::filesystem::path& filepath);
		~IORequestReadFile() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		FileReader& GetResult() { return m_fileReader; }

	private:
		FileReader m_fileReader;
		std::filesystem::path m_filepath;
		IORequestResultCode m_resultCode;
	};
}
