#include "sbpch.h"
#include "Sandbox/Modals/TextureImportModal.h"

#include "Sandbox/Utility/Theme.h"

#include <Volt-Assets/SourceAssetImporters/ImportConfigs.h>

#include <Volt-Application/UI/UIUtility.h>
#include <Volt-Application/UI/UIScopedHelpers.h>
#include <Volt-Application/UI/UIProperties.h>

#include <Volt-Renderer/Renderer.h>
#include <Volt-Renderer/Texture/EnvironmentTexture.h>

#include <CoreUtilities/StringUtility.h>

#include <AssetSystem/SourceAssetManager.h>
#include <AssetSystem/AssetLocks.h>

TextureImportModal::TextureImportModal(const std::string& strId)
	: Modal(strId)
{
}

void TextureImportModal::DrawModalContent()
{
	VT_ENSURE(!m_importFilePaths.empty());

	UI::ScopedStyleFloat indentSpacing{ ImGuiStyleVar_IndentSpacing, { 0.f } };

	ImGui::Text("Importing %s", GetImportTypeStringFromFilepath(m_importFilePaths.back()).c_str());

	if (ImGui::CollapsingHeader("Texture", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (UI::BeginProperties("textureOptions"))
		{
			const Vector<std::string> importTypes =
			{
				"Texture",
				"Environment Texture"
			};

			UI::ComboProperty("ImportType", *reinterpret_cast<int32_t*>(&m_importOptions.importType), importTypes);

			UI::Property("Import Mip Maps", m_importOptions.importMipMaps);
			UI::Property("Generate Mip Maps", m_importOptions.generateMipMaps, "If import mip maps is enabled, but none were found, mip maps will be generated");

			UI::EndProperties();
		}
	}

	{
		UI::ScopedButtonColor importAllColor{ EditorTheme::Buttons::BlueButton };
		if (ImGui::Button("Import All"))
		{
			for (const auto& path : m_importFilePaths)
			{
				Import(path);
			}

			Close();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Import"))
	{
		Import(m_importFilePaths.front());
		m_importFilePaths.erase(m_importFilePaths.begin());

		if (m_importFilePaths.empty())
		{
			Close();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
	{
		Close();
	}
}

void TextureImportModal::OnOpen()
{
	if (m_importFilePaths.empty())
	{
		return;
	}
}

void TextureImportModal::OnClose()
{
	Clear();
}

std::string TextureImportModal::GetImportTypeStringFromFilepath(const std::filesystem::path& filepath)
{
	std::string extension = filepath.extension().string();
	extension.erase(std::remove(extension.begin(), extension.end(), '.'));

	return Utility::ToUpper(extension);
}

void TextureImportModal::Import(const std::filesystem::path filepath)
{
	const std::filesystem::path destinationDirectory = filepath.parent_path();
	const std::string destinationFileName = filepath.stem().string();

	Volt::TextureSourceImportConfig importConfig;
	importConfig.destinationDirectory = destinationDirectory;
	importConfig.destinationFilename = destinationFileName;
	importConfig.generateMipMaps = m_importOptions.generateMipMaps;
	importConfig.importMipMaps = m_importOptions.importMipMaps;

	if (m_importOptions.importType == ImportType::Texture)
	{
		Volt::SourceAssetManager::ImportSourceAsset(filepath, importConfig);
	}
	else if (m_importOptions.importType == ImportType::EnvironmentTexture)
	{
		// If it's an environment texture we create a temporary texture asset,
		// which we use to create the environment texture asset.
		importConfig.createAsMemoryAsset = true;

		auto importCallback = [importConfig](Vector<AssetReference<Volt::Asset>> assets)
		{
			AssetReference<Volt::Asset> textureAsset = assets.back();
			ScopedAssetReferenceLock textureLock{ textureAsset };

			Volt::AssetHandle textureHandle = textureAsset->GetAssetHandle();

			Volt::JobRef job = Volt::JobSystem::CreateJob("Generate Environment Texture", Volt::ExecutionPriority::Latent,
			[textureHandle, importConfig]()
			{
				Volt::Renderer::EnvironmentTextures envTextures = Volt::Renderer::GenerateEnvironmentTextures(textureHandle);
				g_assetManager->CreateAssetAndFile<Volt::EnvironmentTexture>(importConfig.destinationDirectory, importConfig.destinationFilename, envTextures.diffuse, envTextures.specular);
			});

			Volt::JobSystem::RunJob(job);
		};

		Volt::SourceAssetManager::ImportSourceAsset(filepath, importConfig, importCallback);
	}
}

void TextureImportModal::Clear()
{
	m_importOptions = {};
	m_importFilePaths.clear();
}
