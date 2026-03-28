#include "cpch.h"
#include "CoreModule/CommandLineBuilder.h"

#include <FileSystemModule/Filesystem.h>

#include <CoreUtilities/String/StringUtility.h>
#include <CoreUtilities/String/StringBuilder.h>

namespace Volt
{

	CommandLineBuilder::CommandLineBuilder(std::initializer_list<std::pair<String, String>> initializer)
	{
		for (const auto& pair : initializer)
		{
			m_arguments.insert(pair);
		}
	}

	void CommandLineBuilder::AddArgument(const String& key, const String& value /*= ""*/)
	{
		m_arguments[key] = value;
	}

	void CommandLineBuilder::BuildFromArgV(wchar_t** argList, int32_t numArgs)
	{
		m_arguments.clear();

		for (int32_t i = 0; i < numArgs; ++i)
		{
			if (i == 0)
			{
				// First argument will always be the executable filepath
				m_executableFilepath.assign_convert(argList[i], wcslen(argList[i]));
				continue;
			}

			// The standard format for arguments are: -<argname>=<argvalue>. The value is not required, and will be interpreted as a toggle if no value is supplied.
			String argStr(String::CtorConvert(), argList[i], wcslen(argList[i]));

			if (argStr[0] == '-')
			{
				const size_t equalSignPos = argStr.find_first_of('=');
				const bool hasValue = equalSignPos != String::npos;

				String argKey = argStr.substr(1, hasValue ? equalSignPos - 1 : String::npos);
				String argValue = "1";

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
				if (Filesystem::Exists(argStr))
				{
					m_arguments["project"] = argStr;
				}
			}
		}
	}

	void CommandLineBuilder::BuildFromString(const String& string)
	{
		size_t optionOffset = string.find_first_of('-');
		while (optionOffset != String::npos)
		{
			size_t nextOptionOffset = string.find_first_of('-', optionOffset + 1);
			size_t dividerOffset = string.find_first_of('=', optionOffset + 1);

			nextOptionOffset = nextOptionOffset == String::npos ? string.size() : nextOptionOffset;

			String key = string.substr(optionOffset + 1, dividerOffset - optionOffset - 1);
			String value = string.substr(dividerOffset + 1, nextOptionOffset - dividerOffset - 1);

			m_arguments[key] = value;

			optionOffset = string.find_first_of('-', optionOffset + 1);
		}
	}

	String CommandLineBuilder::GetAsString() const
	{
		StringBuilder builder;

		for (const auto& [key, value] : m_arguments)
		{
			builder << "-";
			builder << key;
			if (!value.empty())
			{
				builder << "=";
				builder << value;
			}
			builder << " ";
		}

		return builder.Get();
	}

	WString CommandLineBuilder::GetAsWString() const
	{
		return WString(WString::CtorConvert(), GetAsString());
	}
}
