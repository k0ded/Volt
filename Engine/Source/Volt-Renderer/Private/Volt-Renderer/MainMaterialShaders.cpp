#include "vrpch.h"

#include "Volt-Renderer/MainMaterialShaders.h"

#include <RenderCore/RenderGraph/ShaderRegistry.h>

namespace Volt
{
	VT_REGISTER_SHADER(DefaultDepthPrePassShaderPS, "Engine/Shaders/Source/MaterialShaders/DefaultDepthPrePassPS.hlsl", "MainPS", Pixel);
	VT_REGISTER_SHADER(DefaultBasePassShaderPS, "Engine/Shaders/Source/MaterialShaders/DefaultBasePassPS.hlsl", "MainPS", Pixel);
	VT_REGISTER_SHADER(DefaultTranslucencyPassShaderPS, "Engine/Shaders/Source/MaterialShaders/DefaultTranslucencyPassPS.hlsl", "MainPS", Pixel);

	VT_REGISTER_SHADER(DepthPrePassVS, "Engine/Shaders/Source/MaterialShaders/DepthPrePassVS.hlsl", "MainVS", Vertex);
	VT_REGISTER_SHADER(BasePassVS, "Engine/Shaders/Source/MaterialShaders/BasePassVS.hlsl", "MainVS", Vertex);
	VT_REGISTER_SHADER(TranslucencyPassVS, "Engine/Shaders/Source/MaterialShaders/TranslucencyPassVS.hlsl", "MainVS", Vertex);

	VT_REGISTER_MATERIAL_SHADER(DepthPrePassMaterialShader, DefaultDepthPrePassShaderPS, "Engine/Shaders/Source/MaterialShaders/DepthPrePassPixel.hlsl", "MainPS");
	VT_REGISTER_MATERIAL_SHADER(BasePassMaterialShader, DefaultBasePassShaderPS, "Engine/Shaders/Source/MaterialShaders/BasePassPixel.hlsl", "MainPS");
	VT_REGISTER_MATERIAL_SHADER(TranslucencyPassMaterialShader, DefaultTranslucencyPassShaderPS, "Engine/Shaders/Source/MaterialShaders/TranslucencyPassPixel.hlsl", "MainPS");
}
