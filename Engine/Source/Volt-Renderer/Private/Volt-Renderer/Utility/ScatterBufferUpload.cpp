#include "vrpch.h"

#include "Volt-Renderer/Utility/ScatteredBufferUpload.h"

namespace Volt
{
	REGISTER_SHADER(ScatterUploadCS, "Engine/Shaders/Source/Utility/ScatterUpload_cs.hlsl", "MainCS", Compute);
}
