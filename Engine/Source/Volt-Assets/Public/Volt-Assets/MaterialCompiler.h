#pragma once

#include <CoreUtilities/Core.h>

namespace Volt
{
	class MaterialAsset;

	class MaterialCompiler
	{
	public:
		void CompileMaterial(Ref<MaterialAsset> materialAsset);
	};
}
