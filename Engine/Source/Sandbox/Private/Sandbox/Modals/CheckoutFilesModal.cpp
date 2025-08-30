#include "sbpch.h"
#include "Sandbox/Modals/CheckoutFilesModal.h"

#include <CoreUtilities/FileSystem.h>

#include <AssetSystem/AssetManager.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <Volt-Application/UI/UIScopedHelpers.h>

CheckoutFilesModal::CheckoutFilesModal(const std::string& strId)
	: Modal(strId, ImGuiWindowFlags_None)
{}

bool CheckoutFilesModal::NeedsAction(Volt::AssetHandle handle)
{
	return GetRequiredAction(handle) != RequiredSaveAction::None;
}

RequiredSaveAction CheckoutFilesModal::GetRequiredAction(Volt::AssetHandle handle)
{
	//memory assets need to be created
	if (Volt::AssetManager::IsMemoryAsset(handle))
	{
		return RequiredSaveAction::Create;
	}

	// read-only assets need to be made writeable or get checked out
	{
		std::filesystem::path assetPath = Volt::AssetManager::GetFilePathFromAssetHandle(handle);

		if (!FileSystem::IsWriteable(assetPath))
		{
			return RequiredSaveAction::CheckOut;
		}
	}

	return RequiredSaveAction::None;
}

void CheckoutFilesModal::DrawModalContent()
{
	const int32_t numColumns = static_cast<int32_t>(TableColumns::NUM);
	const ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp |
		ImGuiTableFlags_HighlightHoveredColumn | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders |
		ImGuiTableFlags_Hideable;
	if (ImGui::BeginTable("CheckoutFilesTable", numColumns, flags, ImVec2(ImGui::GetContentRegionAvail().x, 0.f)))
	{
		ImGui::TableSetupColumn(GetTableColumnName(TableColumns::Selected).c_str(), ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_NoHide);
		ImGui::TableSetupColumn(GetTableColumnName(TableColumns::Status).c_str());
		ImGui::TableSetupColumn(GetTableColumnName(TableColumns::Name).c_str());
		ImGui::TableSetupColumn(GetTableColumnName(TableColumns::Type).c_str());
		ImGui::TableSetupColumn(GetTableColumnName(TableColumns::Path).c_str());

		// Sort our data if sort specs have been changed
		if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
		{
			if (sort_specs->SpecsDirty)
			{
				//Sort(sort_specs); // todo: implement
				sort_specs->SpecsDirty = false;
			}
		}

		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		for (int column = 0; column < numColumns; column++)
		{
			ImGui::TableSetColumnIndex(column);
			const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
			ImGui::PushID(column);
			if (static_cast<TableColumns>(column) == TableColumns::Selected)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
				bool allSelected = m_selectedAssets.size() == m_assetsToHandle.size();
				const bool anySelected = !m_selectedAssets.empty();

				if (!allSelected && anySelected)
				{
					ImGuiContext& g = *ImGui::GetCurrentContext();
					g.NextItemData.ItemFlags |= ImGuiItemFlags_MixedValue;
				}
				if (ImGui::Checkbox("##checkall", &allSelected))
				{
					m_selectedAssets.clear();
					if (allSelected)
					{
						m_selectedAssets.insert(m_assetsToHandle.begin(), m_assetsToHandle.end());
					}
				}
				const ImVec2 min = ImGui::GetItemRectMin();
				const ImVec2 max = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::GetColorU32({ 0.4f, 0.4f, 0.4f, 1.f }));

				ImGui::PopStyleVar();
			}
			else
			{
				ImGui::TableHeader(column_name);
			}
			ImGui::PopID();
		}

		for (int32_t i = 0; i < m_assetsToHandle.size(); i++)
		{
			ImGuiID id = ImHashData(&m_assetsToHandle[i], sizeof(Volt::AssetHandle));
			ImGui::PushID(id);
			ImGui::TableNextColumn();
			DrawRowColumn(i, TableColumns::Selected);
			ImGui::TableNextColumn();
			DrawRowColumn(i, TableColumns::Status);
			ImGui::TableNextColumn();
			DrawRowColumn(i, TableColumns::Name);
			ImGui::TableNextColumn();
			DrawRowColumn(i, TableColumns::Type);
			ImGui::TableNextColumn();
			DrawRowColumn(i, TableColumns::Path);
			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	if (ImGui::Button("Mark Writeable"))
	{
		/*for (int32_t index : m_SelectedIndices)
		{
			const std::filesystem::path& path = m_FilePaths[index];

			FileSystem::MakeWriteable(path);
		}*/

		Close();
	}

	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		Close();
	}
}

void CheckoutFilesModal::OnOpen()
{

}

void CheckoutFilesModal::OnClose()
{
	m_assetsToHandle.clear();
	m_assetToRequiredAction.clear();
	m_selectedAssets.clear();
}

void CheckoutFilesModal::FillRequiredActionsMap()
{
	m_assetToRequiredAction.reserve(m_assetsToHandle.size());
	for (const Volt::AssetHandle& handle : m_assetsToHandle)
	{
		m_assetToRequiredAction[handle] = GetRequiredAction(handle);
	}
}

std::string CheckoutFilesModal::GetTableColumnName(TableColumns tableColumn)
{
	std::string result = "Error";
	switch (tableColumn)
	{
		case CheckoutFilesModal::TableColumns::Selected:
			result = "Selected";
			break;
		case CheckoutFilesModal::TableColumns::Status:
			result = "Status";
			break;
		case CheckoutFilesModal::TableColumns::Name:
			result = "Name";
			break;
		case CheckoutFilesModal::TableColumns::Type:
			result = "Type";
			break;
		case CheckoutFilesModal::TableColumns::Path:
			result = "Path";
			break;
	}
	return result;
}

void CheckoutFilesModal::DrawRowColumn(int32_t index, TableColumns tableColumn)
{
	/*glm::vec3 color = (index % 2 == 0) ? */

	const Volt::AssetHandle& handle = m_assetsToHandle[index];
	switch (tableColumn)
	{
		case CheckoutFilesModal::TableColumns::Selected:
		{
			bool selected = m_selectedAssets.contains(handle);
			if (ImGui::Checkbox(std::format("##SelectCheckbox_%d", index).c_str(), &selected))
			{
				if (selected)
				{
					m_selectedAssets.insert(handle);
				}
				else
				{
					m_selectedAssets.erase(handle);
				}
			}
			break;
		}

		case CheckoutFilesModal::TableColumns::Status:
		{
			std::string status = "Error";
			if (m_assetToRequiredAction.contains(handle))
			{
				status = ToString(m_assetToRequiredAction[handle]);
			}
			ImGui::Text(status.c_str());
			break;
		}

		case CheckoutFilesModal::TableColumns::Name:
		{
			const std::string name = Volt::AssetManager::Get().GetAssetRaw(handle)->assetName;
			ImGui::Text(name.c_str());
			break;
		}

		case CheckoutFilesModal::TableColumns::Type:
		{
			AssetType type = Volt::AssetManager::GetAssetTypeFromHandle(handle);
			ImGui::Text(type->GetName().data());
		}
		break;

		case CheckoutFilesModal::TableColumns::Path:
		{
			std::filesystem::path path = Volt::AssetManager::GetFilePathFromAssetHandle(handle);
			ImGui::Text(path.string().c_str());
			break;
		}
	}
}

void CheckoutFilesModal::Sort()
{}
