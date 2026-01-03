#include "cupch.h"

#include "CoreUtilities/Archive/ArchiveVersionRegistry.h"
#include "CoreUtilities/VoltAssert.h"

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
	static ArchiveVersionRegistry registry;
	return registry;
}

void ArchiveVersionRegistry::RegisterVersion(const VoltGUID& guid, int32_t currentVersion, std::string_view name)
{
	VT_ENSURE(!m_registeredVersions.contains(guid));
	m_registeredVersions[guid] = { name, currentVersion };
}

void ArchiveVersionRegistry::UnregisterVersion(const VoltGUID& guid)
{
	if (VT_CHECK(m_registeredVersions.contains(guid)))
	{
		m_registeredVersions.erase(guid);
	}
}
