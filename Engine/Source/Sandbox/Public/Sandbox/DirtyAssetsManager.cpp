#include "sbpch.h"
#include "DirtyAssetsManager.h"

#include "Sandbox/UISystems/ModalSystem.h"

#include "Sandbox/Modals/AssetsModal.h"

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/FileSystem.h>


DirtyAssetsManager DirtyAssetsManager::s_instance{};

DirtyAssetsManager& DirtyAssetsManager::Get()
{
	return s_instance;
}

void DirtyAssetsManager::Initialize()
{
	auto& assetsModal = ModalSystem::AddModal<AssetsModal>("Assets Modal##sandbox");
	m_assetsModalID = assetsModal.GetID();
}

void DirtyAssetsManager::RegisterSaveCustomizationForType(AssetType type, DirtySaveCustomization customization)
{
	VT_ASSERT_MSG(!m_dirtySaveCustomizations.contains(type), std::format("Tried to register a dirty save customization for type '{0}' that already has one. ", type->GetName()));
	m_dirtySaveCustomizations.emplace(type, customization);
}

void DirtyAssetsManager::SaveAssets(bool showSaveDialog, SaveDirtyAssetsFilter filter)
{
	//no dirty assets
	if (m_dirtyAssets.empty())
	{
		return;
	}

	//todo_fabian implement filtering
	filter;

	FrameStackVector<Volt::AssetHandle> assetsToSave;
	assetsToSave.reserve(m_dirtyAssets.size());
	for (const Volt::AssetHandle& dirtyAssetHandle : m_dirtyAssets)
	{
		assetsToSave.push_back(dirtyAssetHandle);
	}

	//save dialog
	if (showSaveDialog)
	{
		AssetsModal& modal = ModalSystem::GetModal<AssetsModal>(m_assetsModalID);
		std::set<Volt::AssetHandle> outSelectedAssetsToSave;
		AssetModalResult result = modal.OpenAssetModalTypeBlocking(AssetModalType::Save, assetsToSave, outSelectedAssetsToSave);

		if (result != AssetModalResult::Save)
		{
			return;
		}

		//user chose to save no assets when prompted
		if (outSelectedAssetsToSave.empty())
		{
			return;
		}

		for (int32_t i = static_cast<int32_t>(assetsToSave.size() - 1); i >= 0; i--)
		{
			if (!outSelectedAssetsToSave.contains(assetsToSave[i]))
			{
				assetsToSave.erase(assetsToSave.begin() + i);
			}
		}
	}

	//create assets dialog
	{
		FrameStackVector<Volt::AssetHandle> assetsNeedCreation;
		for (const Volt::AssetHandle& asset : assetsToSave)
		{
			if (Volt::AssetManager::IsMemoryAsset(asset))
			{
				assetsNeedCreation.push_back(asset);
			}
		}

		AssetsModal& modal = ModalSystem::GetModal<AssetsModal>(m_assetsModalID);
		std::set<Volt::AssetHandle> outSelectedAssetsToCreate;
		AssetModalResult result = modal.OpenAssetModalTypeBlocking(AssetModalType::Create, assetsNeedCreation, outSelectedAssetsToCreate);

		if (result == AssetModalResult::Cancel)
		{
			return;
		}

		if (result == AssetModalResult::Create)
		{
			//todo_fabian create assets here
		}
	}


	//checkout assets dialog
	{
		FrameStackVector<Volt::AssetHandle> readOnlyAssets;
		for (const Volt::AssetHandle& asset : assetsToSave)
		{
			const std::filesystem::path assetPath = Volt::AssetManager::GetFilesystemPath(asset);
			if (!FileSystem::IsWriteable(assetPath))
			{
				readOnlyAssets.push_back(asset);
			}
		}

		if (!readOnlyAssets.empty())
		{

			AssetsModal& modal = ModalSystem::GetModal<AssetsModal>(m_assetsModalID);
			std::set<Volt::AssetHandle> outSelectedAssetsToCheckOut;
			AssetModalResult result = modal.OpenAssetModalTypeBlocking(AssetModalType::CheckOut, readOnlyAssets, outSelectedAssetsToCheckOut);

			if (result == AssetModalResult::CheckOut)
			{
				//todo_fabian check assets out here
			}

			if (result == AssetModalResult::MakeWriteable)
			{
				for (Volt::AssetHandle asset : outSelectedAssetsToCheckOut)
				{
					const std::filesystem::path assetPath = Volt::AssetManager::GetFilesystemPath(asset);
					FileSystem::MakeWriteable(assetPath);
				}
			}

			// remove the assets that are still read-only from the assets to save
			for (int32_t i = static_cast<int32_t>(assetsToSave.size() - 1); i >= 0; i--)
			{
				const std::filesystem::path assetPath = Volt::AssetManager::GetFilesystemPath(assetsToSave[i]);
				if (!FileSystem::IsWriteable(assetPath))
				{
					assetsToSave.erase(assetsToSave.begin() + i);
				}
			}
		}
	}

	//no assets to save
	if (assetsToSave.empty())
	{
		return;
	}

	//SaveAssetsImpl(filter);
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


void DirtyAssetsManager::SaveAssetsImpl(SaveDirtyAssetsFilter filter)
{
	for (const Volt::AssetHandle& dirtyAssetHandle : m_dirtyAssets)
	{
		AssetType type = Volt::AssetManager::GetAssetTypeFromHandle(dirtyAssetHandle);

		if (Volt::AssetManager::IsMemoryAsset(dirtyAssetHandle))
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
