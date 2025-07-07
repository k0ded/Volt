#include "sbpch.h"
#include "DirtyAssetsManager.h"

#include <AssetSystem/AssetManager.h>

DirtyAssetsManager DirtyAssetsManager::s_instance{};

DirtyAssetsManager& DirtyAssetsManager::Get()
{
	return s_instance;
}

void DirtyAssetsManager::RegisterSaveCustomizationForType(AssetType type, DirtySaveCustomizationFn fn)
{
	VT_ASSERT_MSG(!m_dirtySaveCustomizations.contains(type), std::format("Tried to register a dirty save customization for type '{0}' that already has one. ", type->GetName()));
	m_dirtySaveCustomizations.emplace(type, fn);
}


void DirtyAssetsManager::SaveAssets(SaveDirtyAssetsFilter* filter)
{
	//todo_fabian implement filtering
	filter;

	for (const Volt::AssetHandle& dirtyAssetHandle : m_dirtyAssets)
	{
		AssetType type = Volt::AssetManager::GetAssetTypeFromHandle(dirtyAssetHandle);

		if (m_dirtySaveCustomizations.contains(type))
		{
			m_dirtySaveCustomizations[type](dirtyAssetHandle);
		}
		else if (Volt::AssetManager::IsMemoryAsset(dirtyAssetHandle))
		{
			//todo_fabian: make a proper solution for this, maybe a window that pops up
			Volt::AssetManager::SaveMemoryAssetToDirectory(dirtyAssetHandle, "Assets/TEMP/");
		}
		else
		{
			Volt::AssetManager::SaveAsset(dirtyAssetHandle);
		}

		MarkAssetNotDirty(dirtyAssetHandle);
	}
}

bool DirtyAssetsManager::IsAssetDirty(Volt::AssetHandle handle)
{
	return m_dirtyAssets.contains(handle);
}

void DirtyAssetsManager::MarkAssetDirty(Volt::AssetHandle handle)
{
	m_dirtyAssets.insert(handle);
}

void DirtyAssetsManager::MarkAssetNotDirty(Volt::AssetHandle handle)
{
	m_dirtyAssets.erase(handle);
}

const std::set<Volt::AssetHandle>& DirtyAssetsManager::GetDirtyAssets()
{
	return m_dirtyAssets;
}
