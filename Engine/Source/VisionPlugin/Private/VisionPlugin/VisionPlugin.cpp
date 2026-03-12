#include "VisionPlugin.h"

#include <LogModule/Log.h>

VT_REGISTER_COMPONENT(VisionTest);

void VisionPlugin::Initialize()
{
	VT_LOG(Trace, "Hello from VisionPlugin!");
}

void VisionPlugin::Shutdown()
{
}
