#pragma once

#include "AssetSystem/Config.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/Map.h>

namespace Volt
{
	class SourceAssetImporter;
}

class VTAS_API SourceAssetImporterRegistry
{
public:
	Ref<Volt::SourceAssetImporter> RegisterImporter(const Vector<std::string>& assignedExtensions, Ref<Volt::SourceAssetImporter> importer);
	void UnregisterImporter(Ref<Volt::SourceAssetImporter> importer);
	bool ImporterForExtensionExists(const std::string& extension) const;
	Volt::SourceAssetImporter& GetImporterForExtension(const std::string& extension) const;
	
	static SourceAssetImporterRegistry& Get();

private:
	Map<std::string, Ref<Volt::SourceAssetImporter>> m_importers;
};

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_SOURCE_ASSET_IMPORTER(extensions, importerClass) \
	class SourceAssetImporterRegistrar_##importerClass \
	{ \
	public: \
		VT_INLINE SourceAssetImporterRegistrar_##importerClass() \
		{ \
			m_importer = SourceAssetImporterRegistry::Get().RegisterImporter(Vector<std::string>extensions, CreateRef<importerClass>()); \
		} \
		VT_INLINE ~SourceAssetImporterRegistrar_##importerClass() \
		{ \
			SourceAssetImporterRegistry::Get().UnregisterImporter(m_importer); \
		} \
	private: \
		Ref<Volt::SourceAssetImporter> m_importer; \
	} g_sourceAssetImporterRegistrar_##importerClass
