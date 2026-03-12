#include "mcpch.h"
#include "Mosaic/NodeRegistry.h"

Mosaic::NodeRegistry& Mosaic::NodeRegistry::Get()
{
	static Mosaic::NodeRegistry registry;
	return registry;
}
