#include "sbpch.h"

#include "Sandbox/Modals/CreateAssetsModal.h"

#include <CoreUtilities/FileSystem.h>

#include <AssetSystem/AssetManager.h>

#include <SubSystem/SubSystemManager.h>
#include <Volt-Core/Project/ProjectManager.h>

#include <RHIModule/ImGui/FontAwesome.h>

#include <Volt-Application/UI/UIUtility.h>
#include <Volt-Application/UI/UIScopedHelpers.h>

#include <imgui.h>

CreateFilesModal::CreateFilesModal(const std::string& strId)
	: Modal(strId, ImGuiWindowFlags_None)
{}

bool CreateFilesModal::NeedsAction(Volt::AssetHandle handle)
{
	return Volt::AssetManager::IsMemoryAsset(handle);
}

void CreateFilesModal::DrawModalContent()
{
	const uint32_t numColumns = static_cast<int32_t>(CreateFilesTableColumns::NUM);

	const ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp |
		ImGuiTableFlags_HighlightHoveredColumn | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders;
	if (ImGui::BeginTable("CheckoutFilesTable", numColumns, flags, ImVec2(ImGui::GetContentRegionAvail().x, 0.f)))
	{

		ImGui::TableSetupColumn(ToString(CreateFilesTableColumns::Status).c_str(), ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderWidth);
		ImGui::TableSetupColumn(ToString(CreateFilesTableColumns::Name).c_str());
		ImGui::TableSetupColumn(ToString(CreateFilesTableColumns::Type).c_str());
		ImGui::TableSetupColumn(ToString(CreateFilesTableColumns::Path).c_str(), ImGuiTableColumnFlags_NoResize);
		ImGui::TableSetupColumn(ToString(CreateFilesTableColumns::SetPath).c_str(), ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderWidth);

		// Sort our data if sort specs have been changed
		if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
		{
			if (sort_specs->SpecsDirty)
			{
				//Sort(sort_specs); // todo: implement
				sort_specs->SpecsDirty = false;
			}
		}

		//create headers
		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		for (int column = 0; column < numColumns; column++)
		{
			ImGui::TableSetColumnIndex(column);
			const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
			ImGui::PushID(column);

			if (static_cast<CreateFilesTableColumns>(column) == CreateFilesTableColumns::Status ||
				static_cast<CreateFilesTableColumns>(column) == CreateFilesTableColumns::SetPath)
			{
				ImGui::TableHeader("");
			}
			else
			{
				ImGui::TableHeader(column_name);
			}
			ImGui::PopID();
		}

		//create rows
		for (int32_t i = 0; i < m_assetHandles.size(); i++)
		{
			const Volt::AssetHandle& handle = m_assetHandles[i];
			ImGui::PushID(std::format("{}", handle).c_str());

			for (int32_t column = 0; column < numColumns; column++)
			{
				ImGui::TableNextColumn();
				DrawRowColumn(static_cast<CreateFilesTableColumns>(column), handle);
			}
			ImGui::PopID();
		}

		ImGui::EndTable();
	}
}

void CreateFilesModal::OnOpen()
{}

void CreateFilesModal::OnClose()
{}

void CreateFilesModal::DrawRowColumn(CreateFilesTableColumns column, Volt::AssetHandle handle)
{

	switch (column)
	{
		case CreateFilesTableColumns::Status:
		{
			if (m_assetToNewPath.contains(handle))
			{
				UI::ScopedColor color (ImGuiCol_Text, { 0,1,0,1 });
				ImGui::Text(VT_ICON_FA_CHECK);
			}
			else
			{
				{
					UI::ScopedColor color(ImGuiCol_Text, { 1,1,0,1 });
					ImGui::Text(VT_ICON_FA_TRIANGLE_EXCLAMATION);
				}
				UI::SimpleToolTip("Please assign a path to save this asset to.");
			}
			break;
		}
		case CreateFilesTableColumns::Name:
		{
			const std::string assetName = Volt::AssetManager::Get().GetAssetRaw(handle)->assetName;
			ImGui::Text(assetName.c_str());
			break;
		}
		case CreateFilesTableColumns::Type:
		{
			AssetType type = Volt::AssetManager::GetAssetTypeFromHandle(handle);
			ImGui::Text(type->GetName().data());
			break;
		}
		case CreateFilesTableColumns::Path:
		{
			if (m_assetToNewPath.contains(handle))
			{
				std::filesystem::path path = m_assetToNewPath[handle];
				ImGui::Text(path.string().c_str());
			}
			else
			{
				ImGui::Text("No Path Assigned.");
			}
			break;
		}
		case CreateFilesTableColumns::SetPath:
		{
			if (ImGui::Button("...##SetPathButton"))
			{
				FileFilter filter;
				filter.extensions = "vtasset";
				std::filesystem::path pickedPath = FileSystem::SaveFileDialogue({ filter }, Volt::ProjectManager::GetAssetsDirectory());
				if (!pickedPath.empty())
				{
					m_assetToNewPath[handle] = pickedPath;
				}
			}
			break;
		}
		default:
			ImGui::Text("ERROR");
			break;
	}

}
