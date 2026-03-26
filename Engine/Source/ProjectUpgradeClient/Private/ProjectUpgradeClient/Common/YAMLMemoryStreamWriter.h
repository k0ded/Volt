#pragma once

#include "ProjectUpgradeClient/Common/YAMLStreamWriter.h"

#include <CoreModule/DataBuffer.h>

class YAMLMemoryStreamWriter : public YAMLStreamWriter
{
public:
	~YAMLMemoryStreamWriter() = default;

	DataBuffer WriteAndGetBuffer() const;
};
