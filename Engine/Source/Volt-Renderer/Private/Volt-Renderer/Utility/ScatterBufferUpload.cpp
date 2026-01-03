#include "vrpch.h"

#include "Volt-Renderer/Utility/ScatteredBufferUpload.h"

namespace Volt
{
	VT_REGISTER_SHADER(ScatterUploadCS, "Engine/Shaders/Source/Utility/ScatterUpload.hlsl", "MainCS", Compute);
}
