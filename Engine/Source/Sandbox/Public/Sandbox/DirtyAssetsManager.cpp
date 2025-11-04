#include "sbpch.h"
#include "DirtyAssetsManager.h"

#include "Sandbox/UISystems/ModalSystem.h"

#include "Sandbox/Modals/AssetsModal.h"
#include "Sandbox/EditorAssetManager.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/Asset.h>
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
	auto& assetsModal = ModalSystem::AddModal<AssetsModal>("Assets Modal##sandbox");
	m_assetsModalID = assetsModal.GetID();
	m_assetChangedCallbackID = g_assetManager->RegisterAssetUpdatedCallback(AssetTypes::None,
		[this](Volt::AssetHandle assetHandle, Volt::AssetChangedState state)
	{
		OnAssetChanged(assetHandle, state);
	});
}

void DirtyAssetsManager::Shutdown()
{
	g_assetManager->UnregisterAssetUpdatedCallback(AssetTypes::None, m_assetChangedCallbackID);
}

void DirtyAssetsManager::OnAssetChanged(Volt::AssetHandle assetHandle, Volt::AssetChangedState state)
{
	Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(assetHandle);

	if (assetMetadata->IsMemoryAsset())
	{
		return;
	}

	switch (state)
	{
		case Volt::AssetChangedState::Saved:
		case Volt::AssetChangedState::Deleted:
		case Volt::AssetChangedState::Unloaded:
			MarkAssetNotDirty(assetHandle);
			break;

		case Volt::AssetChangedState::Loaded:
		{
			if (!assetMetadata->HasFilepath())
			{
				MarkAssetDirty(assetHandle);
				break;
			}
			else
			{
				MarkAssetNotDirty(assetHandle);
			}
			break;
		}
	}


}

void DirtyAssetsManager::RegisterSaveCustomizationForType(AssetType type, DirtySaveCustomization customization)
{
	VT_ASSERT_MSG(!m_dirtySaveCustomizations.contains(type), std::format("Tried to register a dirty save customization for type '{0}' that already has one. ", type->GetName()));
	m_dirtySaveCustomizations.emplace(type, customization);
}

bool DirtyAssetsManager::SaveAssets(bool showSaveDialog, bool allowDiscardSave, SaveDirtyAssetsFilter filter)
{
	//no dirty assets
	if (m_dirtyAssets.empty())
	{
		return true;
	}

	FrameStackVector<Volt::AssetHandle> assetsToSave;
	assetsToSave.reserve(m_dirtyAssets.size());
	for (auto& [dirtyAssetHandle, assetReference] : m_dirtyAssets)
	{
		if (filter.includeAssetDelegate)
		{
			if (!filter.includeAssetDelegate(dirtyAssetHandle))
			{
				continue;
			}
		}

		assetsToSave.push_back(dirtyAssetHandle);
	}

	if (assetsToSave.empty())
	{
		return true;
	}

	{
		Map<Volt::AssetHandle, std::string> cantSaveAssets;
		for (int32_t i = static_cast<int32_t>(assetsToSave.size() - 1); i >= 0; i--)
		{
			const Volt::AssetHandle& handle = assetsToSave[i];
			Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(handle);

			const AssetType assetType = assetMetadata->type;

			if (!m_dirtySaveCustomizations.contains(assetType))
			{
				continue;
			}

			const DirtySaveCustomization& customization = m_dirtySaveCustomizations[assetType];
			//default behaviour as true
			if (!customization.CanSaveAsset)
			{
				continue;
			}

			std::string outCantReason = "";
			if (!customization.CanSaveAsset(handle, outCantReason))
			{
				//if we arent showing the explicit save dialog, we just remove the asset from assets to save
				if (!showSaveDialog)
				{
					assetsToSave.erase(assetsToSave.begin() + i);
				}
				else
				{
					cantSaveAssets.emplace(handle, outCantReason);
				}
			}
		}

		//save dialog
		if (showSaveDialog)
		{
			AssetsModal& modal = ModalSystem::GetModal<AssetsModal>(m_assetsModalID);
			std::set<Volt::AssetHandle> outSelectedAssetsToSave;

			const AssetModalType assetModalType = allowDiscardSave ? AssetModalType::SaveOrDiscard : AssetModalType::Save;
			const AssetModalResult result = modal.OpenAssetModalTypeBlocking(assetModalType, assetsToSave, outSelectedAssetsToSave, &cantSaveAssets);

			if (result == AssetModalResult::Cancel)
			{
				return false;
			}

			//user chose to save no assets when prompted
			if (outSelectedAssetsToSave.empty() || result == AssetModalResult::Discard)
			{
				return true;
			}

			for (int32_t i = static_cast<int32_t>(assetsToSave.size() - 1); i >= 0; i--)
			{
				if (!outSelectedAssetsToSave.contains(assetsToSave[i]))
				{
					assetsToSave.erase(assetsToSave.begin() + i);
				}
			}
		}
	}

	//create assets dialog
	{
		FrameStackVector<Volt::AssetHandle> assetsNeedUserAssignedPath;
		FrameStackVector<Volt::AssetHandle> assetsNotAllowedUserAssignPath;
		for (int32_t i = static_cast<int32_t>(assetsToSave.size()) - 1; i >= 0; i--)
		{
			const Volt::AssetHandle& asset = assetsToSave[i];
			Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(asset);

			//only assets that dont have a file path need to be created
			if (assetMetadata->HasFilepath())
			{
				continue;
			}

			// check if the asset has a save customization, if it does, check if the user is allowed to assign a path
			// if the user is not allowed to assign a path, we might still be able to save the asset post create
			const AssetType assetType = assetMetadata->type;
			if (m_dirtySaveCustomizations.contains(assetType))
			{
				const DirtySaveCustomization& customization = m_dirtySaveCustomizations[assetType];
				if (!customization.CanUserAssignPath)
				{
					continue;
				}
				//default behaviour as true
				if (customization.CanUserAssignPath && !customization.CanUserAssignPath(asset))
				{
					assetsNotAllowedUserAssignPath.push_back(asset);
					continue;
				}
			}


			//cannot save assets without a path, instead prompt to create
			assetsNeedUserAssignedPath.push_back(asset);
			assetsToSave.erase(assetsToSave.begin() + i);
		}

		Vector<std::pair<Volt::AssetHandle, std::filesystem::path>> assetsToCreate;

		//if there are no assets needing a user assigned path, dont open the modal
		if (!assetsNeedUserAssignedPath.empty())
		{
			AssetsModal& modal = ModalSystem::GetModal<AssetsModal>(m_assetsModalID);
			std::set<Volt::AssetHandle> outSelectedAssetsToCreate;
			AssetModalResult result = modal.OpenAssetModalTypeBlocking(AssetModalType::Create, assetsNeedUserAssignedPath, outSelectedAssetsToCreate);

			if (result == AssetModalResult::Cancel)
			{
				return false;
			}

			if (result == AssetModalResult::Create)
			{
				//populate assets to create
				assetsToCreate.reserve(outSelectedAssetsToCreate.size());
				for (const Volt::AssetHandle& asset : outSelectedAssetsToCreate)
				{
					assetsToCreate.push_back({ asset, modal.GetNewAssetPath(asset) });
				}

				//find what assets were not given a path by the user
				std::set<Volt::AssetHandle> assetsNotAssignedPath;
				for (const Volt::AssetHandle& handle : assetsNeedUserAssignedPath)
				{
					assetsNotAssignedPath.insert(handle);
				}

				//remove the assets that needed a user assigned path but did not get one
				for (int32_t i = static_cast<int32_t>(assetsToSave.size()) - 1; i >= 0; i--)
				{
					if (assetsNotAssignedPath.contains(assetsToSave[i]))
					{
						assetsToSave.erase(assetsToSave.begin() + i);
					}
				}
			}
			CreateAssetsImpl(assetsToCreate);
			assetsToCreate.clear();
		}

		//all the assets that were not allowed to be assigned a user path need to check if they can be saved now with a custom behaviour
		for (const Volt::AssetHandle& handle : assetsNotAllowedUserAssignPath)
		{
			Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(handle);
			const AssetType assetType = assetMetadata->type;
			if (!m_dirtySaveCustomizations.contains(assetType))
			{
				continue;
			}

			const DirtySaveCustomization& customization = m_dirtySaveCustomizations[assetType];

			std::filesystem::path outNewPath = "";
			std::string outCantReason = "CanSaveAssetPostCreateStep was not bound but CanUserAssignPath returned false!!";
			bool canSaveAssetPostCreateStep = false;
			//default behaviour as false
			if (customization.CanSaveAssetPostCreateStep)
			{
				canSaveAssetPostCreateStep = customization.CanSaveAssetPostCreateStep(handle, outNewPath, outCantReason);
			}
			if (!canSaveAssetPostCreateStep)
			{
				VT_LOG(Warning, "Failed to Save asset with handle '{0}' Reason: {1}", handle, outCantReason.c_str());
				auto it = std::find(assetsToSave.begin(), assetsToSave.end(), handle);

				VT_ENSURE_MSG(it != assetsToSave.end(), "assetsToSave is supposed to include all assets that were not allowed an user assigned path at this point.");
				assetsToSave.erase(it);
				continue;
			}

			VT_ENSURE(!outNewPath.empty());
			assetsToCreate.push_back({ handle, outNewPath });
		}

		if (!assetsToCreate.empty())
		{
			CreateAssetsImpl(assetsToCreate);
		}
	}


	//checkout assets dialog
	{
		FrameStackVector<Volt::AssetHandle> readOnlyAssets;
		for (const Volt::AssetHandle& asset : assetsToSave)
		{
			const std::filesystem::path assetPath = g_assetManager->GetAssetFilesystemPath(asset);
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

			if (result == AssetModalResult::Cancel)
			{
				return false;
			}

			if (result == AssetModalResult::CheckOut)
			{
				//todo_fabian check assets out here
			}

			if (result == AssetModalResult::MakeWriteable)
			{
				for (Volt::AssetHandle asset : outSelectedAssetsToCheckOut)
				{
					const std::filesystem::path assetPath = g_assetManager->GetAssetFilesystemPath(asset);
					FileSystem::MakeWriteable(assetPath);
				}
			}

			// remove the assets that are still read-only from the assets to save
			for (int32_t i = static_cast<int32_t>(assetsToSave.size() - 1); i >= 0; i--)
			{
				const std::filesystem::path assetPath = g_assetManager->GetAssetFilesystemPath(assetsToSave[i]);
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
		return true;
	}

	SaveAssetsImpl(assetsToSave);
	return true;
}

bool DirtyAssetsManager::IsAssetDirty(Volt::AssetHandle handle)
{
	return m_dirtyAssets.contains(handle);
}

void DirtyAssetsManager::MarkAssetDirty(Volt::AssetHandle handle)
{
	VT_ENSURE(handle != Volt::Asset::Null());
	AssetReference<Volt::Asset> LoadedAsset;
	VT_ENSURE_MSG(g_assetManager->TryGetTypelessAssetIfLoaded(handle, LoadedAsset), "Tried to mark an unloaded asset as dirty! This is not allowed!");
	m_dirtyAssets[handle] = LoadedAsset;
}

void DirtyAssetsManager::MarkAssetNotDirty(Volt::AssetHandle handle)
{
	if (!m_dirtyAssets.contains(handle))
	{
		return;
	}
	m_dirtyAssets.erase(handle);
}

const Map<Volt::AssetHandle, AssetReference<Volt::Asset>>& DirtyAssetsManager::GetDirtyAssets()
{
	return m_dirtyAssets;
}

void DirtyAssetsManager::SaveAssetsImpl(const FrameStackVector<Volt::AssetHandle>& assetsToSave)
{
	for (const Volt::AssetHandle& handle : assetsToSave)
	{
		Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(handle);

		const AssetType type = assetMetadata->type;
		if (m_dirtySaveCustomizations[type].ShouldDeleteInstead(handle))
		{
			g_editorAssetManager->DeleteAsset(handle);
			continue;
		}

		g_assetManager->SaveAsset(handle);
	}
}

void DirtyAssetsManager::CreateAssetsImpl(const Vector<std::pair<Volt::AssetHandle, std::filesystem::path>>& assetsToCreate)
{
	for (const auto& [asset, path] : assetsToCreate)
	{
		g_assetManager->CreateFileForAsset(asset, path);
	}
}
