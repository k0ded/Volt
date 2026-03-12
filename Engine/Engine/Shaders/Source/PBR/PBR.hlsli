#pragma once

#include "PBR/LightEvaluation.hlsli"

#if 0
struct PBRConstants
{
    vt::UniformBuffer<ViewData> viewData;
    vt::UniformBuffer<DirectionalLightShadowData> directionalLightShadowData;
    
    vt::TypedBuffer<LightDrawData> lights;
    vt::TypedBuffer<int> visibleLights;
    
    vt::TextureSampler linearSampler;
    vt::TextureSampler pointLinearClampSampler;
    vt::TextureSampler shadowSampler;
     
    vt::Tex2D<float4> DFGLuT;
    vt::Tex2DArray<float> directionalLightShadowMap;
    SkyLight skyLight;
};
#endif

StructuredBuffer<LightDrawData> SceneLights;

struct PBRInput
{
    float4 albedo;
    float3 normal;
    float metallic;
    float roughness;
    float3 emissive;
    float ao;
    
    float3 worldPosition;
    uint2 tileId;
};

struct LightOutput
{
    float3 diffuse;
    float3 specular;
};

static PBRInput m_pbrInput;

float EvaluateVisibility(in LightDrawData lightData, float3 worldPosition, float3 normal)
{
    if ((lightData.flags & LightFlags::LF_CastShadows) == 0)
    {
        return 1.f;
    }

    if (lightData.lightType == SceneLightType::SLT_Directional)
    {
        return EvaluateDirectionalShadow_Hard(lightData, normal, worldPosition);
    }

    return 1.f;
}

float3 EvaluateLights(float3 dirToCamera, float ao, uint lightCount)
{
    BRDFInput brdfInput; 
    brdfInput.V = dirToCamera;
    brdfInput.N = m_pbrInput.normal;
    brdfInput.baseColor = m_pbrInput.albedo.rgb;
    brdfInput.roughness = m_pbrInput.roughness;
    brdfInput.metalness = m_pbrInput.metallic;

    float3 radiance = 0.f;

    for (uint i = 0; i < lightCount; i++)
    {
        int lightIndex = GetLightBufferIndex(i, m_pbrInput.tileId);
        if (lightIndex == -1)
        {
            break;
        }

        const LightDrawData light = SceneLights[lightIndex];
        if (light.lightType == SceneLightType::SLT_Point)
        {
            radiance += EvaluatePointLight(light, brdfInput, m_pbrInput.worldPosition);
        }
        else if (light.lightType == SceneLightType::SLT_Spot)
        {
            radiance += EvaluateSpotLight(light, brdfInput, m_pbrInput.worldPosition);
        }
        else if (light.lightType == SceneLightType::SLT_Directional)
        {
            radiance += EvaluateDirectionalLight(light, brdfInput, m_pbrInput.worldPosition);
        }
        else if (light.lightType == SceneLightType::SLT_Sky)
        {
            radiance += EvaluateIBL(brdfInput, light) * ao;
        }
    }

    return radiance;
}

float3 EvaluatePBR(in PBRInput input)
{ 
    m_pbrInput = input;

    const float3 dirToCamera = normalize(View.cameraPosition.xyz - m_pbrInput.worldPosition);
    
    float3 lightOutput = 0.f;
    lightOutput += EvaluateLights(dirToCamera, m_pbrInput.ao, View.lightCount);

    const float3 compositeLighting = lightOutput + m_pbrInput.emissive;
    return compositeLighting;
}