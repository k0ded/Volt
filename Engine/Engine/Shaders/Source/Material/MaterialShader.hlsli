// #NOTE: No pragma and absolute include path, since this file will be inlined.

#include "Material/MaterialCommon.hlsli"
#include "StaticSamplerStates.hlsli"

/*
	This file contains everything required to implement a material shader.
    The file may be included for better IDE support, but will be inlined during compilation.
*/

$(TextureDeclarations)

struct MaterialEvaluationData
{
    float2 texCoords;
};

struct EvaluatedMaterial
{
    float4 albedo;
    float roughness;
    float metallic;
    float3 normal;
    float3 emissive;
    
    void Setup()
    {
        albedo = 1.f;
        roughness = 0.9f;
        metallic = 0.f;
        normal = float3(0.5f, 0.5f, 1.f);
        emissive = 0.f;
    }
};

EvaluatedMaterial EvaluateMaterial(in MaterialEvaluationData evalData)
{
    $(EvaluateMaterial)
}

void EvaluateAlphaMask(inout float alpha)
{
#if MATERIAL_BLEND_MODE == MATERIAL_BLEND_MODE_ALPHA_MASKED
    alpha = step(0.5f, alpha);
    clip(alpha < 0.01f ? -1.f : 1.f);
#endif
}