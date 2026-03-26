#include "aspch.h"
#include "AssetType.h"

VT_REGISTER_ASSET_TYPE(None);

void AssetTypeRegistry::RegisterAssetType(const VoltGUID& guid, AssetType type)
{
	VT_ENSURE(!m_typeMap.contains(guid));
	m_typeMap[guid] = type;
}

void AssetTypeRegistry::UnregisterAssetType(const VoltGUID& guid)
{
	if (VT_CHECK(m_typeMap.contains(guid)))
	{
		m_typeMap.erase(guid);
	}
}

AssetType AssetTypeRegistry::GetTypeFromGUID(const VoltGUID& guid) const
{
	VT_ENSURE(m_typeMap.contains(guid));
	return m_typeMap.at(guid);
}

AssetType AssetTypeRegistry::GetTypeFromExtension(const String& extension) const
{
	for (const auto& [guid, type] : m_typeMap)
	{
		const auto& extensions = type->GetExtensions();
		auto it = std::find(extensions.begin(), extensions.end(), extension);
		if (it != extensions.end())
		{
			return type;
		}
	}

	return AssetTypes::None;
}

AssetTypeRegistry& AssetTypeRegistry::Get()
{
	static AssetTypeRegistry registry;
	return registry;
}
