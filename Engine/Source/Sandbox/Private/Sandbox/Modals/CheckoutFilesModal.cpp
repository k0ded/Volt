#include "sbpch.h"
#include "Sandbox/Modals/CheckoutFilesModal.h"

#include <CoreUtilities/FileSystem.h>

#include <AssetSystem/AssetManager.h>

#include <imgui.h>

CheckoutFilesModal::CheckoutFilesModal(const std::string& strId)
	: Modal(strId)
{
}

void CheckoutFilesModal::SetAssetsToCheckout(Vector<std::filesystem::path> filePaths)
{
	m_FilePaths = filePaths;

	std::sort(m_FilePaths.begin(), m_FilePaths.end(), [](const std::filesystem::path& lhs, const std::filesystem::path& rhs)
		{
			return lhs.filename().string() < rhs.filename().string();
		});
}

void CheckoutFilesModal::SetOnConfirm(std::function<void()> onConfirm)
{
	m_onConfirm = onConfirm;
}

void CheckoutFilesModal::SetOnCancel(std::function<void()> onCancel)
{
	m_onCancel = onCancel;
}

void CheckoutFilesModal::DrawModalContent()
{
	if (ImGui::BeginTable("CheckoutFilesTable", 4))
	{
		ImGui::TableSetupColumn(GetTableColumnName(TableColumns::Selected).c_str(), ImGuiTableColumnFlags_NoResize);
		ImGui::TableSetupColumn(GetTableColumnName(TableColumns::Name).c_str());
		ImGui::TableSetupColumn(GetTableColumnName(TableColumns::Type).c_str());
		ImGui::TableSetupColumn(GetTableColumnName(TableColumns::Path).c_str());

		ImGui::TableNextColumn();
		DrawHeaderForColumn(TableColumns::Selected);
		ImGui::TableNextColumn();
		DrawHeaderForColumn(TableColumns::Name);
		ImGui::TableNextColumn();
		DrawHeaderForColumn(TableColumns::Type);
		ImGui::TableNextColumn();
		DrawHeaderForColumn(TableColumns::Path);

		for (int32_t i = 0; i < m_FilePaths.size(); i++)
		{
			ImGui::TableNextColumn();
			DrawRowColumn(i, TableColumns::Selected);
			ImGui::TableNextColumn();
			DrawRowColumn(i, TableColumns::Name);
			ImGui::TableNextColumn();
			DrawRowColumn(i, TableColumns::Type);
			ImGui::TableNextColumn();
			DrawRowColumn(i, TableColumns::Path);
		}

		ImGui::EndTable();
	}

	if (ImGui::Button("Mark Writeable"))
	{
		for (int32_t index : m_SelectedIndices)
		{
			const std::filesystem::path& path = m_FilePaths[index];

			FileSystem::MakeWriteable(path);
		}

		m_onConfirm();

		Close();
	}

	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		m_onCancel();
		Close();
	}
}

void CheckoutFilesModal::OnOpen()
{
	if (m_FilePaths.empty())
	{
		Close();
	}
}

void CheckoutFilesModal::OnClose()
{
	m_FilePaths.clear();
}

std::string CheckoutFilesModal::GetTableColumnName(TableColumns tableColumn)
{
	std::string result = "Error";
	switch (tableColumn)
	{
	case CheckoutFilesModal::TableColumns::Selected:
		result = "Selected";
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

void CheckoutFilesModal::DrawHeaderForColumn(TableColumns tableColumn)
{
	bool selectAll = !m_SelectedIndices.empty();
	switch (tableColumn)
	{
	case CheckoutFilesModal::TableColumns::Selected:
		if (ImGui::Checkbox("##SelectAllCheckbox", &selectAll))
		{
			if (selectAll)
			{
				for (int32_t i = 0; i < m_FilePaths.size(); i++)
				{
					m_SelectedIndices.insert(i);
				}
			}
			else
			{
				m_SelectedIndices.clear();
			}
		}
		ImGui::TableHeader("");
		break;
	case CheckoutFilesModal::TableColumns::Name:
		ImGui::TableHeader("Name");
		break;
	case CheckoutFilesModal::TableColumns::Type:
		ImGui::TableHeader("Type");
		break;
	case CheckoutFilesModal::TableColumns::Path:
		ImGui::TableHeader("Path");
		break;
	}
}

void CheckoutFilesModal::DrawRowColumn(int32_t index, TableColumns tableColumn)
{
	const std::filesystem::path& path = m_FilePaths[index];
	switch (tableColumn)
	{
	case CheckoutFilesModal::TableColumns::Selected:
	{
		bool selected = m_SelectedIndices.contains(index);
		if (ImGui::Checkbox(std::format("##SelectCheckbox_%d", index).c_str(), &selected))
		{
			if (selected)
			{
				m_SelectedIndices.insert(index);
			}
			else
			{
				m_SelectedIndices.erase(index);
			}
		}
	}
	break;

	case CheckoutFilesModal::TableColumns::Name:
		ImGui::Text(path.stem().string().c_str());
		break;

	case CheckoutFilesModal::TableColumns::Type:
	{
		AssetType type = Volt::AssetManager::GetAssetTypeFromPath(path);
		ImGui::Text(type->GetName().data());
	}
	break;

	case CheckoutFilesModal::TableColumns::Path:
		ImGui::Text(path.string().c_str());
		break;
	}
}
