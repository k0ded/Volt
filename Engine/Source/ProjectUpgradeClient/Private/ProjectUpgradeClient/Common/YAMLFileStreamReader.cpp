#include "ProjectUpgradeClient/Common/YAMLFileStreamReader.h"

#include <fstream>

const bool YAMLFileStreamReader::OpenFile(const Filesystem::Path& filePath)
{
	std::filesystem::path tempPath(filePath.ToWString().begin(), filePath.ToWString().end());

	if (!std::filesystem::exists(tempPath))
	{
		return false;
	}

	std::ifstream file(tempPath);
	if (!file.is_open())
	{
		return false;
	}

	std::stringstream strStream;
	strStream << file.rdbuf();
	file.close();

	m_rootNode = YAML::Load(strStream.str());
	m_currentNode = m_rootNode;

	return true;
}
