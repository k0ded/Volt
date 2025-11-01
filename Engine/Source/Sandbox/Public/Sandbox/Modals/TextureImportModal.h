#pragma once

#include "Sandbox/Modals/Modal.h"

class TextureImportModal final : public Modal
{
public:
	TextureImportModal(const std::string& strId);
	~TextureImportModal() override = default;

	VT_INLINE void SetImportTextures(const Vector<std::filesystem::path>& filePaths) { m_importFilePaths = filePaths; }
	VT_INLINE void SetDestinationDirectory(const std::filesystem::path& destinationDirectory) { m_destinationDirectory = destinationDirectory; }

protected:
	void DrawModalContent() override;
	void OnOpen() override;
	void OnClose() override;

private:
	enum class ImportType
	{
		Texture,
		EnvironmentTexture
	};

	struct ImportOptions
	{
		bool importMipMaps = true;
		bool generateMipMaps = true;
		ImportType importType = ImportType::Texture;
	} m_importOptions;

	std::string GetImportTypeStringFromFilepath(const std::filesystem::path& filepath);

	void Import(const std::filesystem::path& filepath, const std::filesystem::path& destinationDirectory);
	void Clear();

	std::filesystem::path m_destinationDirectory;
	Vector<std::filesystem::path> m_importFilePaths;
};
