#pragma once

#include "AssetSystem/AssetHandle.h"

#include <CoreUtilities/Archive/Archive.h>
#include <CoreUtilities/EnumUtils.h>

namespace Volt
{
	enum class AssetDependencyType : uint8_t
	{
		Hard = 0,
		Soft
	};

	struct AssetDependency
	{
		AssetHandle assetHandle;
		AssetDependencyType dependencyType;

		VT_INLINE friend Archive& operator<<(Archive& archive, AssetDependency& value)
		{
			archive << value.assetHandle;
			archive << value.dependencyType;

			return archive;
		}
	};
}
