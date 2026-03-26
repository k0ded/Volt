#pragma once

#if defined(NLOHMANN_JSON_NAMESPACE_BEGIN)
#error "nlohmann/json.hpp" has been included by another file!
#endif

#define JSON_NOEXCEPTION
#include <nlohmann/json.hpp>
