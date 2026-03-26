#include "ProjectUpgradeClient/Common/YAMLFileStreamWriter.h"

#include <fstream>

YAMLFileStreamWriter::YAMLFileStreamWriter(const Filesystem::Path& targetFilePath)
	: m_targetFilePath(targetFilePath)
{
}

const bool YAMLFileStreamWriter::WriteToDisk()
{
	std::filesystem::path tempPath(m_targetFilePath.ToWString().begin(), m_targetFilePath.ToWString().end());

	std::ofstream fout{ tempPath };
	if (!fout.is_open())
	{
		return false;
	}

	fout << m_emitter.c_str();

	return true;
}
