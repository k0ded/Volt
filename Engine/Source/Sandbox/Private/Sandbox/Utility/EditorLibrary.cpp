#include "sbpch.h"
#include "Utility/EditorLibrary.h"

#include "Sandbox/Window/EditorWindow.h"

#include <AssetSystem/AssetManager.h>

void EditorLibrary::Clear()
{
	s_editors.clear();
}

void EditorLibrary::Sort()
{
	std::sort(s_editors.begin(), s_editors.end(), [](const auto& lhs, const auto& rhs)
	{
		return lhs.editorWindow->GetTitle() < rhs.editorWindow->GetTitle();
	});
}

Ref<EditorWindow> EditorLibrary::GetPanel(const std::string& panelName)
{
	for (const auto& panel : s_editors)
	{
		if (panel.editorWindow->GetTitle() == panelName)
		{
			return panel.editorWindow;
		}
	}

	return nullptr;
}

bool EditorLibrary::OpenAsset(Volt::AssetHandle handle)
{
	Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(handle);

	const AssetType type = assetMetadata->type;
	auto it = std::find_if(s_editors.begin(), s_editors.end(), [type](const auto& lhs) { return lhs.assetType->GetGUID() == type->GetGUID(); });
	if (it == s_editors.end())
	{
		return false;
	}

	AssetReference<Volt::Asset> asset;
	if (!g_assetManager->TryGetTypelessAssetIfLoaded(handle, asset))
	{
		return false;
	}
	it->editorWindow->Open();
	it->editorWindow->OpenAsset(asset);
	return true;
}

Ref<EditorWindow> EditorLibrary::Get(AssetType type)
{
	auto it = std::find_if(s_editors.begin(), s_editors.end(), [type](const auto& lhs) { return lhs.assetType->GetGUID() == type->GetGUID(); });
	if (it == s_editors.end())
	{
		VT_LOG(Error, "Editor for asset not registered!");
		return nullptr;
	}

	return it->editorWindow;
}
