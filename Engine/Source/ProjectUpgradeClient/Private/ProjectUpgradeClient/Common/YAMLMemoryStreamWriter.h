#pragma once

#include "ProjectUpgradeClient/Common/YAMLStreamWriter.h"
#include <CoreUtilities/Buffer/DataBuffer.h>

class YAMLMemoryStreamWriter : public YAMLStreamWriter
{
public:
	~YAMLMemoryStreamWriter() = default;

	DataBuffer WriteAndGetBuffer() const;
};
