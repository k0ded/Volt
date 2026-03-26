#pragma once

#include "Sandbox/Modals/Modal.h"

class FontImportModal final : public Modal
{
public:
	FontImportModal(const String& strId);
	~FontImportModal() override = default;

	VT_INLINE void SetImportFont(const Filesystem::Path& filepath) { m_importFilepath = filepath; }
	VT_INLINE void SetDestinationDirectory(const Filesystem::Path& destinationDirectory) { m_destinationDirectory = destinationDirectory; }

protected:
	void DrawModalContent() override;
	void OnOpen() override;
	void OnClose() override;

private:
	void Import(const Filesystem::Path& filepath, const Filesystem::Path& destinationDirectory);
	void Clear();

	Filesystem::Path m_destinationDirectory;
	Filesystem::Path m_importFilepath;
};
