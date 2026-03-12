#pragma once

#include <AssetSystem/SourceAssetImporter.h>
#include <AssetSystem/SourceAssetImporterRegistry.h>

#include <LogModule/LogCategory.h>

VT_DECLARE_LOG_CATEGORY(LogPNGTextureSourceImporter, LogVerbosity::Trace);

namespace Volt
{
	class PNGTextureSourceImporter : public SourceAssetImporter
	{
	protected:
		Vector<AssetReference<Asset>> ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const override;
		SourceAssetFileInformation GetSourceFileInformation(const std::filesystem::path& filepath) const override;
	};
}
