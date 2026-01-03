#pragma once

#include "ProjectUpgradeClient/Common/YAMLStreamReader.h"

class YAMLFileStreamReader : public YAMLStreamReader
{
public:
	~YAMLFileStreamReader() override = default;
	const bool OpenFile(const std::filesystem::path& filePath);
};
