#pragma once

#include "ProjectUpgradeClient/Common/YAMLStreamReader.h"

class DataBuffer;

class YAMLMemoryStreamReader : public YAMLStreamReader
{
public:
	~YAMLMemoryStreamReader() override = default;

	const bool ReadBuffer(const DataBuffer& buffer);
	const bool ConsumeBuffer(DataBuffer& buffer);
};
