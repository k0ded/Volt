#include "vspch.h"
#include "Volt-Scene/EntityDescSerializationCommon.h"
#include <CoreUtilities/Archive/ArchiveVersionRegistry.h>

namespace Volt::EntityDescSerialization
{
	ArchiveVersionRegistrar g_registerEntityDescArchiveVersion(EntityDescArchiveVersion::guid, EntityDescArchiveVersion::LatestVersion, "EntityDescArchiveVersion");
}
