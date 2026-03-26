#pragma once

#include "CoreModule/JSON/JSONInclude.h"

#include <CoreUtilities/UUID.h>
#include <CoreUtilities/String/VoltString.h>

void to_json(nlohmann::json& j, const UUID32& value)
{
	j = value.Get();
}

void from_json(const nlohmann::json& j, UUID32& value)
{
	value = j.get<uint32_t>();
}

void to_json(nlohmann::json& j, const UUID64& value)
{
	j = value.Get();
}

void from_json(const nlohmann::json& j, UUID64& value)
{
	value = j.get<uint64_t>();
}

void to_json(nlohmann::json& j, const String& value)
{
	std::string temp(value.begin(), value.end());
	j = temp;
}

void from_json(const nlohmann::json& j, String& value)
{
	std::string temp = j.get<std::string>();
	value = String(temp.data(), temp.length());
}
