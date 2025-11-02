#include "sbpch.h"
#include "Utility/EditorUtilities.h"

#include "Sandbox/Utility/AssetBrowserUtilities.h"
#include "Sandbox/Utility/EditorResources.h"
#include "Sandbox/Utility/Theme.h"
#include "Sandbox/DirtyAssetsManager.h"
#include "Sandbox/EditorCommandStack.h"
#include "Sandbox/EditorAssetManager.h"

#include <Volt-Assets/MeshAsset.h>

#include <Volt-Renderer/Texture/Texture2D.h>
#include <Volt-Renderer/Mesh/Mesh.h>

#include <Volt-Animation/Assets/Skeleton.h>
#include <Volt-Application/UI/UIUtility.h>
#include <Volt-Application/UI/UIProperties.h>
#include <Volt-Application/UI/UIScopedHelpers.h>

#include <Volt-Core/Project/ProjectManager.h>

#include <Volt-Scene/Scene.h>
#include <Volt-Scene/EntityDescription.h>

#include <EntitySystem/Entity.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

#include <CoreUtilities/FileSystem.h>

bool EditorUtils::Property(const std::string& text, Volt::AssetHandle& assetHandle, AssetType wantedType)
{
	bool changed = false;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });
	UI::BeginPropertyRow();

	ImGui::TextUnformatted(text.c_str());
	ImGui::TableNextColumn();

	std::string assetFileName = "Null";

	AssetReference<Volt::Asset> asset;
	if (g_assetManager->TryGetTypelessAssetIfLoaded(assetHandle, asset))
	{
		ScopedAssetReferenceLock assetLock{ asset };

		assetFileName = asset->GetAssetName();

		if (wantedType != AssetTypes::None && wantedType != asset->GetType())
		{
			assetHandle = Volt::Asset::Null();
		}
	}

	std::string textId = UI::MakePropertyID();

	changed = UI::DrawItem(ImGui::GetColumnWidth() - 2.f * 25.f, [&]()
	{
		ImGui::InputTextString(textId.c_str(), &assetFileName, ImGuiInputTextFlags_ReadOnly);
		return false;
	});

	Volt::AssetHandle newHandle;
	if (UI::DragDropTarget("ASSET_BROWSER_ITEM", newHandle))
	{
		Volt::ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(newHandle);
		if (assetMetadata->type == wantedType)
		{
			assetHandle = newHandle;
			changed = true;
		}
	}

	ImGui::SameLine();

	std::string buttonId = "X" + UI::MakePropertyID();
	if (ImGui::Button(buttonId.c_str(), { 24.5f, 24.5f }))
	{
		assetHandle = Volt::Asset::Null();
		changed = true;
	}

	ImGui::SameLine();

	std::string selectButtonId = "..." + UI::MakePropertyID();
	std::string popupId = "AssetsPopup" + UI::MakePropertyID();
	const bool startState = s_assetBrowserPopupsOpen[popupId].state;

	if (ImGui::Button(selectButtonId.c_str(), { 24.5f, 24.5f }))
	{
		ImGui::OpenPopup(popupId.c_str());
		s_assetBrowserPopupsOpen[popupId].state = true;
	}

	if (AssetBrowserPopupInternal(popupId, assetHandle, startState, wantedType))
	{
		changed = true;
	}

	UI::EndPropertyRow();
	ImGui::PopStyleVar();

	return changed;
}

bool EditorUtils::AssetBrowserPopupField(const std::string& id, Volt::AssetHandle& assetHandle, AssetType wantedType)
{
	const bool startState = s_assetBrowserPopupsOpen[id].state;
	bool changed = false;

	if (startState != ImGui::IsPopupOpen(id.c_str()) && startState == false)
	{
		s_assetBrowserPopupsOpen[id].state = true;
	}

	if (AssetBrowserPopupInternal(id, assetHandle, startState, wantedType))
	{
		changed = true;
	}

	return changed;
}

bool EditorUtils::SearchBar(std::string& outSearchQuery, bool& outHasSearchQuery, bool setAsActive)
{
	UI::ScopedColor childColor{ ImGuiCol_ChildBg, EditorTheme::DarkGreyBackground };
	UI::ScopedStyleFloat rounding(ImGuiStyleVar_ChildRounding, 2.f);

	constexpr float barHeight = 32.f;
	constexpr float searchBarSize = 22.f;
	bool returnVal = false;

	ImGui::BeginChild("##searchBar", { ImGui::GetContentRegionAvail().x, barHeight });
	{
		UI::ShiftCursor(5.f, 4.f);
		ImGui::Image(UI::GetTextureID(EditorResources::GetEditorIcon(EditorIcon::Search)), { searchBarSize, searchBarSize });

		ImGui::SameLine();

		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x);

		UI::PushID();

		if (setAsActive)
		{
			ImGui::SetKeyboardFocusHere();
		}

		if (UI::InputTextWithHint("", outSearchQuery, "Search..."))
		{
			if (!outSearchQuery.empty())
			{
				outHasSearchQuery = true;
				returnVal = true;
			}
			else
			{
				outHasSearchQuery = false;
			}
		}
		UI::PopID();

		ImGui::PopItemWidth();
	}
	ImGui::EndChild();

	return returnVal;
}

bool EditorUtils::AssetBrowserPopupInternal(const std::string& popupId, Volt::AssetHandle& assetHandle, bool startState, AssetType wantedType)
{
	bool changed = false;

	if (auto it = s_assetBrowserPopups.find(popupId); it == s_assetBrowserPopups.end() && s_assetBrowserPopupsOpen[popupId].state != startState)
	{
		s_assetBrowserPopups.emplace(popupId, CreateRef<AssetBrowserPopup>(popupId, wantedType, assetHandle));
	}

	AssetBrowserPopup::State returnState = AssetBrowserPopup::State::Closed;
	if (s_assetBrowserPopups.find(popupId) != s_assetBrowserPopups.end())
	{
		returnState = s_assetBrowserPopups.at(popupId)->Update();
	}

	if (returnState == AssetBrowserPopup::State::Closed || returnState == AssetBrowserPopup::State::Changed)
	{
		s_assetBrowserPopupsOpen[popupId].state = false;
	}

	if (returnState == AssetBrowserPopup::State::Changed)
	{
		changed = true;
	}

	if (startState != s_assetBrowserPopupsOpen[popupId].state && startState == true)
	{
		s_assetBrowserPopups[popupId] = nullptr;
		s_assetBrowserPopups.erase(popupId);
	}

	return changed;
}

SaveReturnState EditorUtils::SaveFilePopup(const std::string& aId)
{
	SaveReturnState returnState = SaveReturnState::None;
	UI::ScopedStyleFloat rounding{ ImGuiStyleVar_FrameRounding, 2.f };

	if (UI::BeginModal(aId, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Discard"))
		{
			returnState = SaveReturnState::Discard;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Save"))
		{
			returnState = SaveReturnState::Save;
			ImGui::CloseCurrentPopup();
		}

		UI::EndModal();
	}

	return returnState;
}

std::string EditorUtils::GetDuplicatedNameFromEntity(const Volt::Entity& entity)
{
	std::string originalName = entity.GetTag();
	auto lastNumber = originalName.find_last_of("0123456789");
	auto lastUnderscore = originalName.find_last_of('_');

	int32_t currentNumber = 0;

	if (lastNumber != std::string::npos && lastUnderscore != std::string::npos && lastUnderscore < lastNumber)
	{
		std::string currentNumberStr = originalName.substr(lastUnderscore + 1, lastNumber - lastUnderscore);
		originalName = originalName.substr(0, lastUnderscore);
		currentNumber = std::stoi(currentNumberStr);
		currentNumber++;
	}

	originalName += "_" + std::to_string(currentNumber);
	return originalName;
}

void EditorUtils::MarkEntityAsEdited(const Volt::Scene& scene, const Volt::Entity& entity)
{
	const Volt::AssetHandle descHandle = scene.GetEntityDescHandleFromEntityID(entity.GetID());

	AssetReference<Volt::EntityDesc> entityDesc;

	// Make sure the entity is loaded.
	if (g_editorAssetManager->TryGetAssetImmediatelyAndCache(descHandle, entityDesc))
	{
		DirtyAssetsManager::Get().MarkAssetDirty(descHandle);
	}
}

void EditorUtils::MarkEntityAndChildrenAsEdited(const Volt::Scene& scene, const Volt::Entity& entity)
{
	MarkEntityAsEdited(scene, entity);

	for (const auto& child : entity.GetChildren())
	{
		MarkEntityAndChildrenAsEdited(scene, child);
	}
}

void EditorUtils::DestroyEntity(Volt::Scene& scene, const Volt::Entity& entity)
{
	DestroyEntities(scene, { entity });
}

void EditorUtils::DestroyEntities(Volt::Scene& scene, const Vector<Volt::Entity>& entities)
{
	//only the parentmost entities should be called delete on
	FrameStackVector<Volt::Entity> parentmostEntities;
	for (const Volt::Entity& entity : entities)
	{
		bool isParentmost = true;
		for (const Volt::Entity& checking : entities)
		{
			if (entity == checking)
			{
				continue;
			}
			if (entity.IsDistantChildOf(checking))
			{
				isParentmost = false;
				break;
			}
		}
		if (isParentmost)
		{
			parentmostEntities.push_back(entity);
		}

	}

	//pre-collect all entities about to be destroyed to create a editor command
	FrameStackVector<Volt::Entity> toCheck = parentmostEntities;
	Vector<Volt::Entity> allEntitiesBeingDestroyed;
	while (!toCheck.empty())
	{
		Volt::Entity checking = toCheck.back();
		toCheck.pop_back();

		allEntitiesBeingDestroyed.push_back(checking);

		//also check the children of checking 
		Vector<Volt::Entity> children = checking.GetChildren();
		toCheck.append(children.begin(), children.end());
	}

	Ref<ObjectStateCommand> command = CreateRef<ObjectStateCommand>(allEntitiesBeingDestroyed, scene, ObjectStateAction::Delete);
	EditorCommandStack::GetInstance().PushUndo(command);

	//destroy all the parentmose entities
	for (Volt::Entity& entity : parentmostEntities)
	{
		Vector<Volt::AssetHandle> destroyedEntityDescs;
		scene.DestroyEntity(entity, destroyedEntityDescs);
		for (Volt::AssetHandle asset : destroyedEntityDescs)
		{
			DirtyAssetsManager::Get().MarkAssetDirty(asset);
		}
	}
}

bool EditorUtils::IsAssetTypeFileExtension(AssetType assetType, const std::filesystem::path& filepath)
{
	const Vector<std::string>& extensions = assetType->GetExtensions();

	std::string filepathExtension = filepath.extension().string();

	for (const std::string& ext : extensions)
	{
		if (filepathExtension == ext)
		{
			return true;
		}
	}

	return false;
}
