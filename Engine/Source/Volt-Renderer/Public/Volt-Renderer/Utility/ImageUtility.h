#pragma once

#include "Volt-Renderer/Config.h"

#include <RHIModule/Images/Image.h>

namespace Volt::ImageUtility
{
	VTR_API void GenerateMipMaps(RefPtr<RHI::Image> image);
}
