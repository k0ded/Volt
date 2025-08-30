#include "sbpch.h"
#include "DirtyAssetsManager.h"

#include "Sandbox/UISystems/ModalSystem.h"

#include "Sandbox/Modals/CheckoutFilesModal.h"
#include "Sandbox/Modals/CreateAssetsModal.h"

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
	auto& checkoutFilesModal = ModalSystem::AddModal<CheckoutFilesModal>("Checkout Files##sandbox");
	m_checkoutFilesModal = checkoutFilesModal.GetID();

	auto& createFilesModal = ModalSystem::AddModal<CreateFilesModal>("Create Files##sandbox");
	m_createFilesModal = createFilesModal.GetID();
}

void DirtyAssetsManager::RegisterSaveCustomizationForType(AssetType type, DirtySaveCustomization customization)
{
	VT_ASSERT_MSG(!m_dirtySaveCustomizations.contains(type), std::format("Tried to register a dirty save customization for type '{0}' that already has one. ", type->GetName()));
	m_dirtySaveCustomizations.emplace(type, customization);
}

void DirtyAssetsManager::SaveAssets(SaveDirtyAssetsFilter filter)
{
	//todo_fabian implement filtering
	filter;

	Vector<Volt::AssetHandle, FrameStackAllocator::Mark> assetsNeedActions;
	for (const Volt::AssetHandle& dirtyAssetHandle : m_dirtyAssets)
	{
		if (CreateFilesModal::NeedsAction(dirtyAssetHandle))
		{
			assetsNeedActions.push_back(dirtyAssetHandle);
		}
	}

	if (!assetsNeedActions.empty())
	{
		//CheckoutFilesModal& modal = ModalSystem::GetModal<CheckoutFilesModal>(m_checkoutFilesModal);
		CreateFilesModal& modal = ModalSystem::GetModal<CreateFilesModal>(m_createFilesModal);
		modal.SetAssetsToHandle(assetsNeedActions);

		//modal.SetOnConfirm([this, filter]()
		//{
		//	CheckoutFilesModal& modal = ModalSystem::GetModal<CheckoutFilesModal>(m_checkoutFilesModal);
		//	SaveDirtyAssetsFilter confirmFilter = filter;
		//	SaveAssetsImpl(confirmFilter);
		//});

		//modal.SetOnCancel([this, filter]()
		//{
		//	//nothing
		//});

		modal.Open();


		return;
	}

	SaveAssetsImpl(filter);
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
