#pragma once

#include "CoreUtilities/VoltGUID.h"
#include "CoreUtilities/Containers/Map.h"

class ArchiveVersionRegistry
{
public:
	ArchiveVersionRegistry() = default;

	// Returns the current version linked to the GUID, otherwise returns -1
	int32_t TryGetVersion(const VoltGUID& guid);

	static ArchiveVersionRegistry& Get();

private:
	friend class ArchiveVersionRegistrar;
	void RegisterVersion(const VoltGUID& guid, int32_t currentVersion, std::string_view name);

	struct VersionInfo
	{
		std::string_view name;
		int32_t currentVersion;
	};

	Map<VoltGUID, VersionInfo> m_registeredVersions;
};

class ArchiveVersionRegistrar
{
public:
	ArchiveVersionRegistrar(const VoltGUID& guid, int32_t currentVersion, std::string_view name)
	{
		ArchiveVersionRegistry::Get().RegisterVersion(guid, currentVersion, name);
	}
};
