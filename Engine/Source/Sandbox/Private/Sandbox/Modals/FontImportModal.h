#pragma once

#include "Sandbox/Modals/Modal.h"

class FontImportModal final : public Modal
{
public:
	FontImportModal(const std::string& strId);
	~FontImportModal() override = default;

	VT_INLINE void SetImportFont(const std::filesystem::path& filepath) { m_importFilepath = filepath; }
	VT_INLINE void SetDestinationDirectory(const std::filesystem::path& destinationDirectory) { m_destinationDirectory = destinationDirectory; }

protected:
	void DrawModalContent() override;
	void OnOpen() override;
	void OnClose() override;

private:
	void Import(const std::filesystem::path& filepath, const std::filesystem::path& destinationDirectory);
	void Clear();

	std::filesystem::path m_destinationDirectory;
	std::filesystem::path m_importFilepath;
};
