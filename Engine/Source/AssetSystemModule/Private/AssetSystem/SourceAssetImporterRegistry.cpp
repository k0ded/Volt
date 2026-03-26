#include "aspch.h"
#include "AssetSystem/SourceAssetImporterRegistry.h"

Ref<Volt::SourceAssetImporter> SourceAssetImporterRegistry::RegisterImporter(const Vector<String>& assignedExtensions, Ref<Volt::SourceAssetImporter> importer)
{
	for (const auto& ext : assignedExtensions)
	{
		VT_ENSURE(!m_importers.contains(ext));
		m_importers[ext] = importer;
	}

	return importer;
}

void SourceAssetImporterRegistry::UnregisterImporter(Ref<Volt::SourceAssetImporter> importer)
{
	Vector<String> extsToRemove;
	for (const auto& [ext, importerInstance] : m_importers)
	{
		extsToRemove.emplace_back(ext);
	}

	for (const auto& ext : extsToRemove)
	{
		m_importers.erase(ext);
	}
}

bool SourceAssetImporterRegistry::ImporterForExtensionExists(const String& extension) const
{
	return m_importers.contains(extension);
}

Volt::SourceAssetImporter& SourceAssetImporterRegistry::GetImporterForExtension(const String& extension) const
{
	VT_ENSURE(m_importers.contains(extension));
	return *m_importers.at(extension);
}

SourceAssetImporterRegistry& SourceAssetImporterRegistry::Get()
{
	static SourceAssetImporterRegistry registry;
	return registry;
}
