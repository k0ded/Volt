#pragma once

#include "CoreUtilities/UUID.h"
#include "CoreUtilities/JSON/JSONInclude.h"

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
