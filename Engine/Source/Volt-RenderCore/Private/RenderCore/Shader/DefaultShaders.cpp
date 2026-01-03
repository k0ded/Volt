#include "rcpch.h"
#include "RenderCore/Shader/DefaultShaders.h"
#include "RenderCore/RenderGraph/ShaderRegistry.h"
#include "RenderCore/CopyToSwapchainShaders.h"

namespace Volt
{
	VT_REGISTER_SHADER(OpaqueDefaultPixelPS, "Engine/Shaders/Source/RenderPipelineLegacy/OpaqueDefaultPixel.hlsl", "MainPS", Pixel);
	VT_REGISTER_SHADER(FullscreenTriangleVS, "Engine/Shaders/Source/Utility/FullscreenTriangle.hlsl", "MainVS", Vertex);
	VT_REGISTER_SHADER(CopyToSwapchain_SDR, "Engine/Shaders/Source/SwapchainConversion/CopyToSwapchain_SDR.hlsl", "MainPS", Pixel);
	VT_REGISTER_SHADER(CopyToSwapchain_HDR, "Engine/Shaders/Source/SwapchainConversion/CopyToSwapchain_HDR.hlsl", "MainPS", Pixel);
}
