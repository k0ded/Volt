#include "UpgradesRegistry.h"

Volt::UpgradesRegistry& Volt::UpgradesRegistry::Get()
{
	static UpgradesRegistry registry;
	return registry;
}
