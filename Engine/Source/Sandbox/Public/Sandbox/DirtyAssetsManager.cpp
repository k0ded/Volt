#include "sbpch.h"
#include "DirtyAssetsManager.h"

#include "Sandbox/UISystems/ModalSystem.h"

#include "Sandbox/Modals/AssetsModal.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/Events/AssetEvents.h>

#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/FileSystem.h>



VT_REGISTER_SUBSYSTEM(DirtyAssetsManager, Default, Engine, 0);

DirtyAssetsManager* DirtyAssetsManager::s_instance = nullptr;

DirtyAssetsManager& DirtyAssetsManager::Get()
{
	VT_ENSURE(s_instance != nullptr);
	return *s_instance;
}

DirtyAssetsManager::DirtyAssetsManager()
{
	VT_ENSURE(s_instance == nullptr);
	s_instance = this;
}

DirtyAssetsManager::~DirtyAssetsManager()
{
	s_instance = nullptr;
}


void DirtyAssetsManager::Initialize()
{
	RegisterEventListeners();

	auto& assetsModal = ModalSystem::AddModal<AssetsModal>("Assets Modal##sandbox");
	m_assetsModalID = assetsModal.GetID();
}

void DirtyAssetsManager::Shutdown()
{
}

void DirtyAssetsManager::RegisterEventListeners()
{
	RegisterListener<Volt::AssetCreatedEvent>(VT_BIND_EVENT_FN(DirtyAssetsManager::OnAssetCreated));
	RegisterListener<Volt::AssetSavedEvent>(VT_BIND_EVENT_FN(DirtyAssetsManager::OnAssetSaved));
}

bool DirtyAssetsManager::OnAssetCreated(Volt::AssetCreatedEvent& e)
{
	const Volt::AssetHandle& handle = e.GetAssetHandle();
	if (Volt::AssetManager::IsMemoryAsset(handle))
	{
		return false;
	}
	MarkAssetDirty(handle);
	return false;
}

bool DirtyAssetsManager::OnAssetSaved(Volt::AssetSavedEvent& e)
{
	MarkAssetNotDirty(e.GetAssetHandle());
	return false;
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
			if (!Volt::AssetManager::HasFilePath(asset))
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
			Vector<std::pair<Volt::AssetHandle, std::filesystem::path>> assetsToCreate;
			assetsToCreate.reserve(outSelectedAssetsToCreate.size());
			for (const Volt::AssetHandle& asset : outSelectedAssetsToCreate)
			{
				assetsToCreate.push_back({ asset, modal.GetNewAssetPath(asset) });
			}
			CreateAssetsImpl(assetsToCreate);
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
	if (!m_dirtyAssets.contains(handle))
	{
		return;
	}
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

		Volt::AssetManager::SaveAsset(dirtyAssetHandle);

		MarkAssetNotDirty(dirtyAssetHandle);
	}
}

void DirtyAssetsManager::CreateAssetsImpl(Vector<std::pair<Volt::AssetHandle, std::filesystem::path>> assetsToCreate)
{
	for (const auto& [asset, path] : assetsToCreate)
	{
		Volt::AssetManager::CreateFileForAsset(asset, path);

		MarkAssetNotDirty(asset);
	}
}
