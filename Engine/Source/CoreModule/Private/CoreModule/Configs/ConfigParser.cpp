#include "cpch.h"
#include "CoreModule/Configs/ConfigParser.h"

namespace ConfigParser
{
	inline String Trim(StringView str)
	{
		constexpr const char* Whitespace = " \t\r\n";

		size_t start = str.find_first_not_of(Whitespace);
		if (start == StringView::npos)
		{
			return {};
		}

		size_t end = str.find_last_not_of(Whitespace);
		return String(str.substr(start, end - start + 1));
	}

	inline String StripInlineComments(StringView line)
	{
		bool inQuotes = false;

		for (size_t i = 0; i < line.size(); ++i)
		{
			char c = line[i];

			if (c == '"' && (i == 0 || line[i - 1] != '\\'))
			{
				inQuotes = !inQuotes;
			}

			if (!inQuotes && (c == ';' || c == '#'))
			{
				return String(line.substr(0, i));
			}
		}

		return String(line);
	}

	inline String UnescapeString(StringView str)
	{
		String out;
		out.reserve(str.size());

		for (size_t i = 0; i < str.size(); ++i)
		{
			if (str[i] == '\\' && i + 1 < str.size())
			{
				char next = str[++i];
				switch (next)
				{
					case 'n': out.push_back('\n'); break;
					case 't': out.push_back('\t'); break;
					case '"': out.push_back('"'); break;
					case '\\': out.push_back('\\'); break;
					default: out.push_back(next); break;
				}
			}
			else
			{
				out.push_back(str[i]);
			}
		}

		return out;
	}

	inline ConfigValue ParseValueFromString(StringView str)
	{
		String value = Trim(str);

		if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
		{
			StringView inner = StringView(value).substr(1, value.size() - 2);
			return ConfigValue(UnescapeString(inner));
		}

		if (str == "true")
		{
			return ConfigValue(true);
		}
		else if (str == "false")
		{
			return ConfigValue(false);
		}

		// Integer
		char* end = nullptr;
		long i = std::strtol(str.data(), &end, 10);

		if (*end == '\0')
		{
			return ConfigValue(static_cast<int32_t>(i));
		}

		// Float
		float f = std::strtof(str.data(), &end);
		if (*end == '\0')
		{
			return ConfigValue(f);
		}

		return ConfigValue(String(str));
	}

	inline void ParseLine(const String& rawLine, Config& config, ConfigSection*& currentSection)
	{
		String noComment = StripInlineComments(rawLine);
		String line = Trim(noComment);

		if (line.empty())
		{
			return;
		}

		// Comment
		if (line.front() == ';' || line.front() == '#')
		{
			return;
		}

		// Section
		if (line.front() == '[' && line.back() == ']')
		{
			String sectionName = Trim(StringView(line).substr(1, line.size() - 2));
			currentSection = &config.Section(sectionName);
			return;
		}

		// Key = value
		size_t equals = line.find('=');
		if (equals == String::npos || !currentSection)
		{
			return;
		}

		String key = Trim(StringView(line).substr(0, equals));
		String value = Trim(StringView(line).substr(equals + 1));

		currentSection->Set(key, ParseValueFromString(value));
	}

	Config ParseConfigFromString(const String& str)
	{
		Config config;
		ConfigSection* currentSection = nullptr;

		String line;

		size_t offset = 0;
		while (offset != String::npos)
		{
			offset = SplitStringWithDelimiter(str, line, offset, '\n');

			if (!line.empty())
			{
				ParseLine(line, config, currentSection);
			}
		}

		return config;
	}
}
