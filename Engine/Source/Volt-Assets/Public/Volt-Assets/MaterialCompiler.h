#pragma once

#include "Volt-Assets/Config.h"

#include <CoreUtilities/Core.h>

#include <LogModule/LogCategory.h>

VT_DECLARE_LOG_CATEGORY_EXPORT(VTASSETS_API, LogMaterialCompiler, LogVerbosity::Trace);

namespace Volt
{
	class MaterialAsset;

	class MaterialCompiler
	{
	public:
		void CompileMaterial(Ref<MaterialAsset> materialAsset);
	};
}
