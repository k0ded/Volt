#include "sbpch.h"

#include <Volt-Assets/SourceAssetImporters/ImportConfigs.h>

#include <AssetSystem/SourceAssetManager.h>

#include "Sandbox/Modals/FontImportModal.h"

#include <imgui.h>

FontImportModal::FontImportModal(const std::string& strId)
	: Modal(strId)
{

}

void FontImportModal::DrawModalContent()
{
	if (ImGui::Button("Import"))
	{
		Import(m_importFilepath, m_destinationDirectory);
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
	{
		Close();
	}
}

void FontImportModal::OnOpen()
{}

void FontImportModal::OnClose()
{
	Clear();
}

void FontImportModal::Import(const std::filesystem::path& filepath, const std::filesystem::path& destinationDirectory)
{
	VT_ENSURE(!destinationDirectory.empty());

	const std::string destinationFileName = filepath.stem().string();

	Volt::FontSourceImportConfig importConfig;
	importConfig.destinationDirectory = destinationDirectory;
	importConfig.destinationFilename = destinationFileName;

	Volt::SourceAssetManager::ImportSourceAsset(filepath, importConfig);
}

void FontImportModal::Clear()
{
	m_importFilepath.clear();
}
