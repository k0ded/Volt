#pragma once

#include "AssetSystem/AssetHandle.h"

#include <CoreUtilities/Filesystem/Path.h>

namespace Volt
{
	struct SourceAssetImportConfig
	{
		Filesystem::Path destinationDirectory;
		String destinationFilename;

		/*
			Set to true if the asset should only live in memory (will not be serialized to disk)
		*/
		bool createAsMemoryAsset = false;

		/*
			Can be assigned if the asset should be imported to target a specific asset handle.
		*/
		AssetHandle targetAssetHandle = 0;
	};
}
