#pragma once

#include "CoreModule/Configs/Config.h"
#include "CoreModule/Config.h"

namespace ConfigParser
{
	VTC_API Config ParseConfigFromString(const String& str);
}
