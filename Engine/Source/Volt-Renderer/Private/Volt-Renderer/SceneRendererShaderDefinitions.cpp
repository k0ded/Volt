#include "vrpch.h"

#include "Volt-Renderer/SceneRendererShaderDefinitions.h"

namespace Volt
{
	REGISTER_SHADER(DepthPrePassVS, "Engine/Shaders/Source/RenderPipelineLegacy/DepthPrePass.hlsl", "MainVS", Vertex);
	REGISTER_SHADER(DepthPrePassPS, "Engine/Shaders/Source/RenderPipelineLegacy/DepthPrePass.hlsl", "MainPS", Pixel);

	REGISTER_SHADER(BasePassVS, "Engine/Shaders/Source/RenderPipelineLegacy/GenerateGBuffer.hlsl", "MainVS", Vertex);
	REGISTER_SHADER(BasePassPS, "Engine/Shaders/Source/RenderPipelineLegacy/GenerateGBuffer.hlsl", "MainPS", Pixel);
}
