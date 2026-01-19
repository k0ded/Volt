#include "vrpch.h"

#include "Volt-Renderer/MainMaterialShaders.h"

namespace Volt
{
	VT_REGISTER_MATERIAL_SHADER(DepthPrePassMaterialShader, "Engine/Shaders/Source/MaterialShaders/DepthPrePassPixel.hlsl", "MainPS");
	VT_REGISTER_MATERIAL_SHADER(BasePassMaterialShader, "Engine/Shaders/Source/MaterialShaders/GenerateGBufferPixel.hlsl", "MainPS");
}
