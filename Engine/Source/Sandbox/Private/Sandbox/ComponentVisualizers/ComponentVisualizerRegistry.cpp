#include "sbpch.h"

#include "Sandbox/ComponentVisualizers/ComponentVisualizerRegistry.h"

ComponentVisualizerRegistry& ComponentVisualizerRegistry::Get()
{
	static ComponentVisualizerRegistry registry;
	return registry;
}
