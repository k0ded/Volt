#include "sspch.h"

#include "SubSystem/SubSystemRegistry.h"

SubSystemRegistry::SubSystemRegistry()
{
}

SubSystemRegistry::~SubSystemRegistry()
{
}

SubSystemRegistry& SubSystemRegistry::Get()
{
	static SubSystemRegistry registry;
	return registry;
}
