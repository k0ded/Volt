#pragma once

#include "Sandbox/Modals/Modal.h"

#include <CoreUtilities/Filesystem/Path.h>

class TextureImportModal final : public Modal
{
public:
	TextureImportModal(const String& strId);
	~TextureImportModal() override = default;

	VT_INLINE void SetImportTextures(const Vector<Filesystem::Path>& filePaths) { m_importFilePaths = filePaths; }
	VT_INLINE void SetDestinationDirectory(const Filesystem::Path& destinationDirectory) { m_destinationDirectory = destinationDirectory; }

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

	String GetImportTypeStringFromFilepath(const Filesystem::Path& filepath);

	void Import(const Filesystem::Path& filepath, const Filesystem::Path& destinationDirectory);
	void Clear();

	Filesystem::Path m_destinationDirectory;
	Vector<Filesystem::Path> m_importFilePaths;
};
