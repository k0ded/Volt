#include "vrpch.h"

#include "Volt-Renderer/SceneRendererShaderDefinitions.h"

namespace Volt
{
	VT_REGISTER_SHADER(DepthPrePassVS, "Engine/Shaders/Source/RenderPipelineLegacy/DepthPrePass.hlsl", "MainVS", Vertex);
	VT_REGISTER_SHADER(DepthPrePassPS, "Engine/Shaders/Source/RenderPipelineLegacy/DepthPrePass.hlsl", "MainPS", Pixel);

	VT_REGISTER_SHADER(BasePassVS, "Engine/Shaders/Source/RenderPipelineLegacy/GenerateGBuffer.hlsl", "MainVS", Vertex);
	VT_REGISTER_SHADER(BasePassPS, "Engine/Shaders/Source/RenderPipelineLegacy/GenerateGBuffer.hlsl", "MainPS", Pixel);
}
