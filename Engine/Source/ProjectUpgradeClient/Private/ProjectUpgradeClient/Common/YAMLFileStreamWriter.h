#pragma once

#include "ProjectUpgradeClient/Common/YAMLStreamWriter.h"

class YAMLFileStreamWriter : public YAMLStreamWriter
{
public:
	YAMLFileStreamWriter(const Filesystem::Path& targetFilePath);
	~YAMLFileStreamWriter() override = default;

	const bool WriteToDisk();

private:
	Filesystem::Path m_targetFilePath;
};
