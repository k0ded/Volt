#include "cupch.h"

#include "CoreUtilities/CommandLineBuilder.h"
#include "CoreUtilities/StringUtility.h"
#include "CoreUtilities/FileSystem.h"

#include <sstream>

namespace Volt
{

	void CommandLineBuilder::AddArgument(const std::string& key, const std::string& value /*= ""*/)
	{
		m_arguments[key] = value;
	}

	void CommandLineBuilder::BuildFromArgV(wchar_t** argList, int32_t numArgs)
	{
		m_arguments.clear();

		// We skip the first argument as it will be the executable filepath.
		for (int32_t i = 1; i < numArgs; ++i)
		{
			// The standard format for arguments are: -<argname>=<argvalue>. The value is not required, and will be interpreted as a toggle if no value is supplied.
			std::string argStr = Utility::ToString(argList[i]);

			if (argStr[0] == '-')
			{
				const size_t equalSignPos = argStr.find_first_of('=');
				const bool hasValue = equalSignPos != std::string::npos;

				std::string argKey = argStr.substr(1, hasValue ? equalSignPos - 1 : std::string::npos);
				std::string argValue = "1";

				if (hasValue)
				{
					argValue = argStr.substr(equalSignPos + 1, argStr.size() - equalSignPos);
				}

				m_arguments[argKey] = argValue;
			}
			// Index 1 is a special case, if it's a filepath that exists and is without any option, it should be used as the project filepath,
			// otherwise it's parsed as a normal argument.
			else if (i == 1)
			{
				if (FileSystem::Exists(argStr))
				{
					m_arguments["project"] = argStr;
				}
			}
		}
	}

	std::string CommandLineBuilder::GetAsString() const
	{
		std::stringstream sstream;

		for (const auto& [key, value] : m_arguments)
		{
			sstream << "-";
			sstream << key;
			if (!value.empty())
			{
				sstream << "=";
				sstream << value;
			}
			sstream << " ";
		}

		return sstream.str();
	}

	std::wstring CommandLineBuilder::GetAsWString() const
	{
		return Utility::ToWString(GetAsString());
	}
}
