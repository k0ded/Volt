#pragma once

#include "AssetSystem/Asset_New.h"

namespace Volt
{
	template<typename T>
	concept VoltAssetType = std::is_base_of_v<Volt::Asset_New, T>;

}
