#include "cupch.h"

#include "CoreUtilities/Archive/ArchiveVersionRegistry.h"

ArchiveVersionRegistry g_archiveVersionRegistry;

int32_t ArchiveVersionRegistry::TryGetVersion(const VoltGUID& guid)
{
	if (m_registeredVersions.contains(guid))
	{
		return m_registeredVersions.at(guid).currentVersion;
	}

	return -1;
}

ArchiveVersionRegistry& ArchiveVersionRegistry::Get()
{
	return g_archiveVersionRegistry;
}

void ArchiveVersionRegistry::RegisterVersion(const VoltGUID& guid, int32_t currentVersion, std::string_view name)
{
	VT_ENSURE(!m_registeredVersions.contains(guid));
	m_registeredVersions[guid] = { name, currentVersion };
}
