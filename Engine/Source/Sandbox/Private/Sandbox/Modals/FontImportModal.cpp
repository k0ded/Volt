#include "sbpch.h"

#include <Volt-Assets/SourceAssetImporters/ImportConfigs.h>

#include <AssetSystem/SourceAssetManager.h>

#include "Sandbox/Modals/FontImportModal.h"

#include <imgui.h>

FontImportModal::FontImportModal(const String& strId)
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

void FontImportModal::Import(const Filesystem::Path& filepath, const Filesystem::Path& destinationDirectory)
{
	VT_ENSURE(!destinationDirectory.IsEmpty());

	const String destinationFileName = filepath.Stem().ToString();

	Volt::FontSourceImportConfig importConfig;
	importConfig.destinationDirectory = destinationDirectory;
	importConfig.destinationFilename = destinationFileName;

	Volt::SourceAssetManager::ImportSourceAsset(filepath, importConfig);
}

void FontImportModal::Clear()
{
	m_importFilepath.Clear();
}
