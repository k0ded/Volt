#include "vrpch.h"

#include "Volt-Renderer/MainMaterialShaders.h"

#include <RenderCore/RenderGraph/ShaderRegistry.h>

namespace Volt
{
	VT_REGISTER_SHADER(DefaultDepthPrePassShaderPS, "Engine/Shaders/Source/MaterialShaders/DefaultDepthPrePassPS.hlsl", "MainPS", Pixel);
	VT_REGISTER_SHADER(DefaultBasePassShaderPS, "Engine/Shaders/Source/MaterialShaders/DefaultBasePassPS.hlsl", "MainPS", Pixel);

	VT_REGISTER_MATERIAL_SHADER(DepthPrePassMaterialShader, DefaultDepthPrePassShaderPS, "Engine/Shaders/Source/MaterialShaders/DepthPrePassPixel.hlsl", "MainPS");
	VT_REGISTER_MATERIAL_SHADER(BasePassMaterialShader, DefaultBasePassShaderPS, "Engine/Shaders/Source/MaterialShaders/GenerateGBufferPixel.hlsl", "MainPS");
}
