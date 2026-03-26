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
		Vector<AssetReference<Asset>> ImportInternal(const Filesystem::Path& filepath, const void* config, const SourceAssetUserImportData& userData) const override;
		SourceAssetFileInformation GetSourceFileInformation(const Filesystem::Path& filepath) const override;
	};
}
