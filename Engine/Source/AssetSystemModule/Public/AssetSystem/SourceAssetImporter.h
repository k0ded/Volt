#pragma once

#include "AssetSystem/Config.h"
#include "AssetSystem/AssetReference.h"

#include <LogModule/LogCommon.h>
#include <CoreUtilities/Containers/Vector.h>

#include <functional>
#include <filesystem>

namespace Volt
{
	class Asset;

	struct SourceAssetUserImportData
	{
		std::function<void(float)> progressCallback;
		std::function<void(const String&, LogVerbosity)> messageCallback;
		std::function<String()> passwordRequiredCallback;

		VT_INLINE void OnInfo(const String& string) const
		{
			if (messageCallback)
			{
				messageCallback(string, LogVerbosity::Info);
			}
		}

		VT_INLINE void OnWarning(const String& string) const
		{
			if (messageCallback)
			{
				messageCallback(string, LogVerbosity::Warning);
			}
		}

		VT_INLINE void OnError(const String& string) const
		{
			if (messageCallback)
			{
				messageCallback(string, LogVerbosity::Error);
			}
		}

		VT_INLINE String OnPasswordRrquired() const
		{
			if (passwordRequiredCallback)
			{
				return passwordRequiredCallback();
			}

			return "";
		}
	};

	struct SourceAssetFileInformation
	{
		String fileVersion;
		String fileCreator;
		String fileCreatorApplication;
		String fileUnits;
		String fileAxisDirection;

		bool hasSkeleton;
		bool hasMesh;
		bool hasAnimation;
	};

	class VTAS_API SourceAssetImporter
	{
	public:
		virtual SourceAssetFileInformation GetSourceFileInformation(const Filesystem::Path& filepath) const = 0;

		template<typename ConfigType>
		Vector<AssetReference<Asset>> Import(const Filesystem::Path& filepath, const ConfigType& config, const SourceAssetUserImportData& userData = {})
		{
			return ImportInternal(filepath, reinterpret_cast<const void*>(&config), userData);
		}

	protected:
		virtual Vector<AssetReference<Asset>> ImportInternal(const Filesystem::Path& filepath, const void* config, const SourceAssetUserImportData& userData) const = 0;
	};
}
