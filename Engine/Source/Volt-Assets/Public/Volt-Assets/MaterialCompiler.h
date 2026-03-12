#pragma once

#include "Volt-Assets/Config.h"
#include "Volt-Assets/MaterialAsset.h"

#include <AssetSystem/AssetReference.h>

#include <LogModule/LogCategory.h>
#include <CoreUtilities/Core.h>

VT_DECLARE_LOG_CATEGORY_EXPORT(VTASSETS_API, LogMaterialCompiler, LogVerbosity::Trace);

namespace Volt
{
	class MaterialCompiler
	{
	public:
		void CompileMaterial(AssetReference<MaterialAsset> materialAsset);
	};
}
