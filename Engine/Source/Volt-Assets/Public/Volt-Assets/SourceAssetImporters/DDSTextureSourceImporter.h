#pragma once

#include <AssetSystem/SourceAssetImporter.h>
#include <AssetSystem/SourceAssetImporterRegistry.h>

#include <LogModule/LogCategory.h>

VT_DECLARE_LOG_CATEGORY(LogDDSTextureSourceImporter, LogVerbosity::Trace);

namespace Volt
{
	class DDSTextureSourceImporter final : public SourceAssetImporter
	{
	protected:
		Vector<AssetReference<Asset>> ImportInternal(const Filesystem::Path& filepath, const void* config, const SourceAssetUserImportData& userData) const override;
		SourceAssetFileInformation GetSourceFileInformation(const Filesystem::Path& filepath) const override;
	};
}
