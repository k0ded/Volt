#include "ProjectUpgradeClient/Common/YAMLMemoryStreamReader.h"

#include <CoreUtilities/Buffer/DataBuffer.h>

const bool YAMLMemoryStreamReader::ReadBuffer(const DataBuffer& buffer)
{
	if (!buffer.IsValid())
	{
		return false;
	}

	std::string tempStr;
	tempStr.resize(buffer.GetSize());

	memcpy_s(tempStr.data(), tempStr.size(), buffer.As<void>(), buffer.GetSize());

	m_rootNode = YAML::Load(tempStr);
	m_currentNode = m_rootNode;

	return true;
}

const bool YAMLMemoryStreamReader::ConsumeBuffer(DataBuffer& buffer)
{
	const bool success = ReadBuffer(buffer);
	buffer.Release();

	return success;
}
