#include "ProjectUpgradeClient/Common/YAMLMemoryStreamWriter.h"

DataBuffer YAMLMemoryStreamWriter::WriteAndGetBuffer() const
{
	DataBuffer buffer{ m_emitter.size() };
	buffer.Copy(m_emitter.c_str(), m_emitter.size());
	return buffer;
}
