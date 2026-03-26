#pragma once

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/String/VoltString.h>

#include <algorithm>

namespace Utility
{
	inline String ToLower(const String& str)
	{
		String newStr(str);
		std::transform(str.begin(), str.end(), newStr.begin(), [](unsigned char c) { return (uint8_t)std::tolower((int32_t)c); });

		return newStr;
	}

	inline String ToUpper(const String& str)
	{
		String newStr(str);
		std::transform(str.begin(), str.end(), newStr.begin(), [](unsigned char c) { return (uint8_t)std::toupper((int32_t)c); });

		return newStr;
	}

	inline String RemoveTrailingZeroes(const String& string)
	{
		String newStr = string;
		newStr.erase(newStr.find_last_not_of('0') + 1, String::npos);

		if (!newStr.empty() && newStr.back() == '.')
		{
			newStr.pop_back();
		}

		return newStr;
	}

	inline const Vector<String> SplitStringsByCharacter(const String& src, const char character)
	{
		Vector<String> result;
		String token;
		
		size_t offset = 0;
		while (offset != String::npos)
		{
			offset = SplitStringWithDelimiter(src, token, offset, character);
			if (!token.empty())
			{
				result.emplace_back(token);
			}
		}

		return result;
	}

	inline const Vector<WString> SplitStringsByCharacter(const WString& src, const wchar_t character)
	{
		Vector<WString> result;
		WString token;

		size_t offset = 0;
		while (offset != WString::npos)
		{
			offset = SplitStringWithDelimiter(src, token, offset, character);
			if (!token.empty())
			{
				result.emplace_back(token);
			}
		}

		return result;
	}

	inline const String ReplaceCharacter(const String& src, const char oldCharacter, const char newCharacter)
	{
		String temp = src;
		std::replace(temp.begin(), temp.end(), oldCharacter, newCharacter);

		return temp;
	}

	template<typename T, typename std::enable_if_t<std::is_integral<T>::value, bool> = true>
	inline const String ToStringWithMetricPrefixCharacterForBytes(const T aValue)
	{
		if (aValue > 1000000000000)
		{
			return FormatString("{:.1f} TB", aValue / 1000000000000.f);
		}
		else if (aValue > 1000000000)
		{
			return FormatString("{:.1f} GB", aValue / 1000000000.f);
		}
		else if (aValue > 1000000)
		{
			return FormatString("{:.1f} MB", aValue / 1000000.f);
		}
		else if (aValue > 1000)
		{
			return FormatString("{: .1f} kB", aValue / 1000.f);
		}
		else
		{
			return FormatString("{} B", aValue);
		}
	}

	template<typename T, typename std::enable_if_t<std::is_integral<T>::value, bool> = true>
	inline const String ToStringWithThousandSeparator(const T aValue)
	{
		String result = "";

		String valueAsString = FormatString("{}", aValue);

		//traverse the string backwards
		int counter = 0;
		for (int32_t i = static_cast<int32_t>(valueAsString.size()) - 1; i >= 0; i--)
		{
			result.push_back(valueAsString[i]);
			counter++;
			if (counter >= 3 && i != 0)
			{
				counter = 0;
				result.push_back(',');
			}
		}
		std::reverse(result.begin(), result.end());

		return result;
	}
}
