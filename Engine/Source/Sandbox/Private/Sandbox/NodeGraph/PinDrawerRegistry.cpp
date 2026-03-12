#include "sbpch.h"
#include "Sandbox/NodeGraph/PinDrawerRegistry.h"

PinDrawerRegistry& PinDrawerRegistry::Get()
{
	static PinDrawerRegistry registry;
	return registry;
}
