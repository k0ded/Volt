#include "cupch.h"
#include "CoreUtilities/Archive/Archive.h"

Archive::Archive(bool isLoading)
	: m_isLoading(isLoading)
{

}

void Archive::UseVersion(const VoltGUID& guid)
{
	if (m_isLoading)
	{
		return;
	}

	auto it = std::find_if(m_versions.begin(), m_versions.end(), [&guid](const VersionInfo& versionInfo)
	{
		return versionInfo.guid == guid;
	});

	if (it == m_versions.end())
	{
		m_versions.emplace_back(guid);
	}
}

int32_t Archive::GetVersion(const VoltGUID& guid) const
{
	if (!m_isLoading)
	{
		return -1;
	}

	auto it = std::find_if(m_versions.begin(), m_versions.end(), [&guid](const VersionInfo& versionInfo)
	{
		return versionInfo.guid == guid;
	});

	if (it != m_versions.end())
	{
		return it->version;
	}

	return -1;
}
