#pragma once

#include <filesystem>

namespace Volt
{
	struct SourceAssetImportConfig
	{
		std::filesystem::path destinationDirectory;
		std::string destinationFilename;

		bool createAsMemoryAsset = false;
	};
}
