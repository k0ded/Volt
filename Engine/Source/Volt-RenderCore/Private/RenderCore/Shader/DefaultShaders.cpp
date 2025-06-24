#include "rcpch.h"
#include "RenderCore/Shader/DefaultShaders.h"
#include "RenderCore/RenderGraph/ShaderRegistry.h"

namespace Volt
{
	REGISTER_SHADER(OpaqueDefaultPixelPS, "Engine/Shaders/Source/RenderPipelineLegacy/OpaqueDefaultPixel.hlsl", "MainPS", Pixel);
	REGISTER_SHADER(FullscreenTriangleVS, "Engine/Shaders/Source/Utility/FullscreenTriangle.hlsl", "MainVS", Vertex);
}
