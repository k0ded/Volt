#pragma once

#include "Containers/Vector.h"

#include <CoreUtilities/Concepts.h>
#include <CoreUtilities/String/VoltString.h>

#include <unordered_map>
#include <map>
#include <cassert>

namespace Utils
{
	/// <summary>
	/// Class for automatically generating enum to string and string to enum functions. DO NOT CALL THESE FUNCTIONS DIRECTLY.
	/// </summary>
	class EnumUtil
	{
	public:
		template<Enum EnumType>
		static String ToString(uint64_t aEnumValue)
		{
			assert(myRegistry.contains(typeid(EnumType).name()) && "Tried to Convert enum to string with an enum that is not registered!");

			return myRegistry[typeid(EnumType).name()][aEnumValue];
		}

		//to enum
		template<Enum EnumType>
		static EnumType ToEnum(String aEnumValue)
		{
			assert(myRegistry.contains(typeid(EnumType).name()) && "Tried to convert string to enum that has not been registered!");
			const auto& enumMap = myRegistry[typeid(EnumType).name()];
			for (const auto& pair : enumMap)
			{
				if (pair.second == aEnumValue)
				{
					return static_cast<EnumType>(pair.first);
				}
			}
			//VT_LOG(LogSeverity::Error, "Tried to convert string to enum that does not exist! EnumName: {0}, EnumValue: {1}", typeid(EnumType).name(), aEnumValue);
			return static_cast<EnumType>(0);
		}

		static bool RegisterEnum(const String& name, const String& definitionData)
		{
			const String allLetters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

			size_t stringOffset = definitionData.find_first_of(allLetters);
			Vector<String> enumNames;
			Vector<uint64_t> enumValues;
			while (stringOffset != String::npos)
			{
				size_t memberNameEndOffset = definitionData.find_first_not_of(allLetters, stringOffset);
				if (memberNameEndOffset == String::npos)
				{
					memberNameEndOffset = definitionData.size();
				}

				String memberName = definitionData.substr(stringOffset, memberNameEndOffset - stringOffset);
				uint64_t memberValue = 0;

				assert(!memberName.empty() && "When Registering an enum, you must provide a name for each member");

				//find the comma
				size_t endOfMemberOffset = definitionData.find_first_of(',', memberNameEndOffset);
				if (endOfMemberOffset == String::npos)
				{
					//this is the last member so instead take the end of the string
					endOfMemberOffset = definitionData.size();
				}

				String memberValueString = definitionData.substr(memberNameEndOffset, endOfMemberOffset - memberNameEndOffset);

				//if the value string contains a = then we need to parse it else we just increment the highest value so far
				if (memberValueString.find('=') != String::npos)
				{
					//remove whitespace
					memberValueString.erase(std::remove_if(memberValueString.begin(), memberValueString.end(), isspace), memberValueString.end());

					//remove the =
					memberValueString.erase(std::remove(memberValueString.begin(), memberValueString.end(), '='), memberValueString.end());

					//if the value is hex
					if (memberValueString.find('x') != String::npos)
					{
						memberValue = StoUll(memberValueString, nullptr, 16);
					}
					//if the value is binary
					else if (memberValueString.find('b') != String::npos)
					{
						memberValue = StoUll(memberValueString, nullptr, 2);
					}
					//if the value is bitshifted to the left
					else if (memberValueString.find('<') != String::npos)
					{
						//split string along "<<"
						const size_t bitshiftOffset = memberValueString.find_first_of('<');
						String base = memberValueString.substr(0, bitshiftOffset);
						String shift = memberValueString.substr(bitshiftOffset + 2, memberValueString.size() - bitshiftOffset - 2);

						memberValue = StoUll(base, nullptr, 10) << (shift, nullptr, 10);
					}
					//if the value is bitshifted to the right
					else if (memberValueString.find('>') != String::npos)
					{
						//split string along ">>"
						const size_t bitshiftOffset = memberValueString.find_first_of('>');
						String base = memberValueString.substr(0, bitshiftOffset);
						String shift = memberValueString.substr(bitshiftOffset + 2, memberValueString.size() - bitshiftOffset - 2);

						memberValue = StoUll(base, nullptr, 10) >> StoUll(shift, nullptr, 10);
					}
					//if the value is a number
					else
					{
						memberValue = StoUll(memberValueString);
					}
				}
				else
				{
					memberValue = std::numeric_limits<uint64_t>::max();
				}

				stringOffset = definitionData.find_first_of(allLetters, endOfMemberOffset);
				enumNames.push_back(memberName);
				enumValues.push_back(memberValue);
			}
			for (size_t i = 0; i < enumValues.size(); i++)
			{
				if (enumValues[i] == std::numeric_limits<uint64_t>::max())
				{
					if (i == 0)
					{
						enumValues[i] = 0;
					}
					else
					{
						enumValues[i] = enumValues[i - 1] + 1;
					}
				}
			}
			auto& enumMap = myRegistry[name];
			for (size_t i = 0; i < enumNames.size(); i++)
			{
				enumMap.insert({ enumValues[i], enumNames[i] });
			}

			return true;
		}



	private:
		//<EnumName, <EnumValue, EnumString>>
		inline static std::unordered_map<String, std::map<uint64_t, String>> myRegistry;
	};
}

template<Enum T> inline static T ToEnum(const String& aEnumString)
{
	return Utils::EnumUtil::ToEnum<T>(aEnumString);
};

#define CREATE_ENUM_TYPED(enumName, type, ...)\
enum class enumName : type \
{ \
	__VA_ARGS__ \
}; \
inline static bool enumName##_enum_reg = Utils::EnumUtil::RegisterEnum(typeid(enumName).name(), #__VA_ARGS__); \
inline static String ToString(enumName aEnumValue) \
{ \
	return Utils::EnumUtil::ToString<enumName>(static_cast<uint64_t>(aEnumValue)); \
}

#define CREATE_ENUM(enumName, ...) CREATE_ENUM_TYPED(enumName, uint32_t, __VA_ARGS__)

#define VT_SETUP_ENUM_SERIALIZE_OPERATOR(enumType) \
	VT_INLINE Archive& operator<<(Archive& archive, enumType& value) \
	{ \
		using UnderlyingType = std::underlying_type_t<enumType>; \
		UnderlyingType& tempValue = *reinterpret_cast<UnderlyingType*>(&value); \
		archive << tempValue; \
		return archive; \
	} \

template<Enum T>
inline static constexpr bool EnumValueContainsFlag(const T& value, const T& flag)
{
	return (value & flag) != static_cast<T>(0);
}


template<Enum T, typename... Args>
inline static constexpr bool EnumValueContainsAnyFlag(const T& value, Args&&... args)
{
	return (((value & args) != static_cast<T>(0)) || ...);
}
