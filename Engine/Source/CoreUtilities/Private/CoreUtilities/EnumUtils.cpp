#include "cupch.h"

#include "CoreUtilities/EnumUtils.h"

namespace Utils
{
	EnumUtil::RegistryMap& EnumUtil::GetRegistry()
	{
		static RegistryMap registryMap;
		return registryMap;
	}
}
