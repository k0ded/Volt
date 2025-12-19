#include "UpgradesRegistry.h"

Volt::UpgradesRegistry g_upgradesRegistry;

Volt::UpgradesRegistry& Volt::UpgradesRegistry::Get()
{
	return g_upgradesRegistry;
}
