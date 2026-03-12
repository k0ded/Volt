#include "cupch.h"

#include "CoreUtilities/Configs/ConfigParser.h"

#include <sstream>

namespace ConfigParser
{
	inline std::string Trim(std::string_view str)
	{
		constexpr const char* Whitespace = " \t\r\n";

		size_t start = str.find_first_not_of(Whitespace);
		if (start == std::string_view::npos)
		{
			return {};
		}

		size_t end = str.find_last_not_of(Whitespace);
		return std::string(str.substr(start, end - start + 1));
	}

	inline std::string StripInlineComments(std::string_view line)
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
				return std::string(line.substr(0, i));
			}
		}

		return std::string(line);
	}

	inline std::string UnescapeString(std::string_view str)
	{
		std::string out;
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

	inline ConfigValue ParseValueFromString(std::string_view str)
	{
		std::string value = Trim(str);

		if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
		{
			std::string_view inner = std::string_view(value).substr(1, value.size() - 2);
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

		return ConfigValue(std::string(str));
	}

	inline void ParseLine(const std::string& rawLine, Config& config, ConfigSection*& currentSection)
	{
		std::string noComment = StripInlineComments(rawLine);
		std::string line = Trim(noComment);

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
			std::string sectionName = Trim(std::string_view(line).substr(1, line.size() - 2));
			currentSection = &config.Section(sectionName);
			return;
		}

		// Key = value
		size_t equals = line.find('=');
		if (equals == std::string::npos || !currentSection)
		{
			return;
		}

		std::string key = Trim(std::string_view(line).substr(0, equals));
		std::string value = Trim(std::string_view(line).substr(equals + 1));

		currentSection->Set(key, ParseValueFromString(value));
	}

	Config ParseConfigFromString(const std::string& str)
	{
		std::istringstream iss(str);

		Config config;
		ConfigSection* currentSection = nullptr;

		std::string line;
		while (std::getline(iss, line))
		{
			ParseLine(line, config, currentSection);
		}

		return config;
	}
}
