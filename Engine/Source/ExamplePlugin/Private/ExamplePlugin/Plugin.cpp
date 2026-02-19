#include "Plugin.h"

#include <LogModule/Log.h>
#include <CoreUtilities/NewOverload.inl>

void ExamplePlugin::Initialize()
{
	VT_LOG(Trace, "Hello from ExamplePlugin!");
}

void ExamplePlugin::Shutdown()
{
}
