#pragma once

#include "CoreUtilities/Configs/Config.h"
#include "CoreUtilities/Config.h"

namespace ConfigParser
{
	VTCOREUTIL_API Config ParseConfigFromString(const std::string& str);
}
