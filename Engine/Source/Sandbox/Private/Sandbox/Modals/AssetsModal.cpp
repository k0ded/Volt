#include "sbpch.h"

#include "Sandbox/Modals/AssetsModal.h"

#include <CoreUtilities/FileSystem.h>

#include <AssetSystem/AssetManager.h>

#include <SubSystem/SubSystemManager.h>
#include <Volt-Core/Project/ProjectManager.h>

#include <Volt-ImGui/FontAwesome.h>

#include <Volt-Application/UI/UIUtility.h>
#include <Volt-Application/UI/UIScopedHelpers.h>

#include <imgui.h>

AssetsModal::AssetsModal(const std::string& strId)
	: Modal(strId, ImGuiWindowFlags_None)
	, m_result(AssetModalResult::None)
	, m_assetModalType(AssetModalType::None)
	, m_numColumns(0)
{}

void AssetsModal::OnOpen()
{
	VT_ASSERT_MSG(m_assetModalType != AssetModalType::None, "Please call OpenAssetModalTypeBlocking instead to open this modal.");

	//count number of columns the type has
	m_numColumns = 0;
	for (int32_t column = 0; column < static_cast<int>(CreateFilesTableColumns::NUM); column++)
	{
		if (ShouldColumnExist(static_cast<CreateFilesTableColumns>(column)))
		{
			m_numColumns++;
		}
	}

	m_result = AssetModalResult::None;
}

void AssetsModal::OnClose()
{
	m_assetModalType = AssetModalType::None;
	VT_ASSERT(m_result != AssetModalResult::None);
}

AssetModalResult AssetsModal::OpenAssetModalTypeBlockingImpl(AssetModalType inAssetModalType, std::set<Volt::AssetHandle>& outSelectedAssets)
{
	m_assetModalType = inAssetModalType;
	OpenBlocking();

	if (m_assetModalType == AssetModalType::Create)
	{
		for (std::pair<Volt::AssetHandle, std::filesystem::path> pair : m_assetToNewPath)
		{
			m_selectedAssets.insert(pair.first);
		}
	}

	outSelectedAssets = m_selectedAssets;
	return GetResult();
}

bool AssetsModal::ShouldColumnExist(CreateFilesTableColumns column)
{
	switch (column)
	{
		case CreateFilesTableColumns::Selected:
			return m_assetModalType != AssetModalType::Create;
			break;

		case CreateFilesTableColumns::Status:
		case CreateFilesTableColumns::SetPath:
			return m_assetModalType == AssetModalType::Create;
			break;
	}
	return true;
}

void AssetsModal::SetupTableColumns()
{
	for (int32_t column = 0; column < static_cast<int>(CreateFilesTableColumns::NUM); column++)
	{
		const CreateFilesTableColumns columnEnum = static_cast<CreateFilesTableColumns>(column);
		if (ShouldColumnExist(columnEnum))
		{
			SetupTableColumn(columnEnum);
		}
	}
}

void AssetsModal::SetupTableColumn(CreateFilesTableColumns column)
{
	ImGuiTableFlags flags = 0;
	switch (column)
	{
		case CreateFilesTableColumns::Selected:
			flags = ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderWidth;
			break;

		case CreateFilesTableColumns::Status:
			flags = ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderWidth;
			break;

		case CreateFilesTableColumns::Name:
			break;
		case CreateFilesTableColumns::Type:
			break;
		case CreateFilesTableColumns::Path:
			break;

		case CreateFilesTableColumns::SetPath:
			flags = ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderWidth;
			break;
		case CreateFilesTableColumns::NUM:
			break;
	}

	ImGui::TableSetupColumn(ToString(column).c_str(), flags);
}

void AssetsModal::DrawModalContent()
{
	constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp |
		ImGuiTableFlags_HighlightHoveredColumn | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders;
	if (ImGui::BeginTable("AssetsModalTable", m_numColumns, flags, ImVec2(ImGui::GetContentRegionAvail().x, 0.f)))
	{
		SetupTableColumns();

		// Sort our data if sort specs have been changed
		if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
		{
			if (sort_specs->SpecsDirty)
			{
				//Sort(sort_specs); // todo: implement
				sort_specs->SpecsDirty = false;
			}
		}

		//draw headers
		{
			ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
			int32_t columnIndex = 0;
			for (int32_t column = 0; column < static_cast<int>(CreateFilesTableColumns::NUM); column++)
			{
				CreateFilesTableColumns columnEnum = static_cast<CreateFilesTableColumns>(column);
				if (!ShouldColumnExist(columnEnum))
				{
					continue;
				}
				ImGui::TableSetColumnIndex(columnIndex);
				columnIndex++;

				DrawHeaderRowColumn(columnEnum);
			}
		}

		//draw rows
		for (int32_t row = 0; row < m_assetHandles.size(); row++)
		{
			const Volt::AssetHandle& handle = m_assetHandles[row];
			ImGui::PushID(std::format("{}", handle).c_str());

			for (int32_t column = 0; column < static_cast<int>(CreateFilesTableColumns::NUM); column++)
			{
				CreateFilesTableColumns columnEnum = static_cast<CreateFilesTableColumns>(column);
				if (!ShouldColumnExist(columnEnum))
				{
					continue;
				}

				ImGui::TableNextColumn();
				DrawRowColumn(columnEnum, handle);
			}

			ImGui::PopID();
		}


		ImGui::EndTable();
	}
	//draw decision buttons
	DrawDecisionButtons();

	//if we have a result, we can close the modal
	if (m_result != AssetModalResult::None)
	{
		Close();
	}
}

void AssetsModal::DrawHeaderRowColumn(CreateFilesTableColumns column)
{
	const char* column_name = ImGui::TableGetColumnName(); // Retrieve name passed to TableSetupColumn()
	ImGui::PushID(column_name);
	switch (column)
	{
		case CreateFilesTableColumns::Selected:
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

			bool allSelected = m_selectedAssets.size() == m_assetHandles.size();
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
					m_selectedAssets.insert(m_assetHandles.begin(), m_assetHandles.end());
				}
			}

			//give the checkbox a background to be able to see it
			const ImVec2 min = ImGui::GetItemRectMin();
			const ImVec2 max = ImGui::GetItemRectMax();
			ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::GetColorU32({ 0.4f, 0.4f, 0.4f, 1.f }));

			ImGui::PopStyleVar();
			break;
		}
		case CreateFilesTableColumns::Status:
		case CreateFilesTableColumns::SetPath:
			ImGui::TableHeader("");
			break;
		case CreateFilesTableColumns::NUM:
			break;
		default:
			ImGui::TableHeader(column_name);
			break;
	}
	ImGui::PopID();
}

void AssetsModal::DrawRowColumn(CreateFilesTableColumns column, Volt::AssetHandle handle)
{
	switch (column)
	{
		case CreateFilesTableColumns::Selected:
		{
			bool selected = m_selectedAssets.contains(handle);
			if (ImGui::Checkbox(std::format("##SelectCheckbox_%d", handle).c_str(), &selected))
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

		case CreateFilesTableColumns::Status:
		{
			if (m_assetToNewPath.contains(handle))
			{
				UI::ScopedColor color(ImGuiCol_Text, { 0,1,0,1 });
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
			if (m_assetModalType == AssetModalType::Create)
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
			}
			else
			{
				const std::filesystem::path path = Volt::AssetManager::GetFilePathFromAssetHandle(handle);
				ImGui::Text(path.string().c_str());
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

void AssetsModal::DrawDecisionButtons()
{
	ImGui::PushStyleColor(ImGuiCol_Button, PrimaryButtonColor);
	switch (m_assetModalType)
	{
		case AssetModalType::Create:
			if (ImGui::Button("Create"))
			{
				m_result = AssetModalResult::Create;
			}
			break;
		case AssetModalType::Save:
			if (ImGui::Button("Save"))
			{
				m_result = AssetModalResult::Save;
			}
			break;
		case AssetModalType::CheckOut:
			if (ImGui::Button("Check Out"))
			{
				m_result = AssetModalResult::CheckOut;
			}
			break;
	}
	ImGui::PopStyleColor(); // pop primary button color


	if (m_assetModalType == AssetModalType::CheckOut)
	{
		ImGui::SameLine();
		if (ImGui::Button("Make Writeable"))
		{
			m_result = AssetModalResult::MakeWriteable;
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		m_result = AssetModalResult::Cancel;
	}
}

