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

float3 EvaluateLights(float3 dirToCamera, uint lightCount)
{
    float3 output = 0.f;

    BRDFInput brdfInput; 
    brdfInput.V = dirToCamera;
    brdfInput.N = m_pbrInput.normal;
    brdfInput.diffuseColor = CalculateDiffuseColor(m_pbrInput.albedo.rgb, m_pbrInput.metallic);
    brdfInput.f0 = CalculateF0(m_pbrInput.albedo.rgb, m_pbrInput.metallic);
    brdfInput.f90 = CalculateF90(m_pbrInput.albedo.rgb, m_pbrInput.metallic);
    brdfInput.roughness = m_pbrInput.roughness;
    brdfInput.metalness = m_pbrInput.metallic;

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
            output += EvaluatePointLight(light, brdfInput, m_pbrInput.worldPosition);
        }
        else if (light.lightType == SceneLightType::SLT_Spot)
        {
            output += EvaluateSpotLight(light, brdfInput, m_pbrInput.worldPosition);
        }
        else if (light.lightType == SceneLightType::SLT_Directional)
        {
            output += EvaluateDirectionalLight(light, brdfInput, m_pbrInput.worldPosition);
        }
        //else if (light.lightType == SceneLightType::SLT_Sky)
        //{
        //    output += EvaluateIBL(brdfInput, m_pbrConstants.DFGLuT, m_pbrConstants.linearSampler, m_pbrConstants.skyLight, light);
        //}
    }

    return output;
}

float3 EvaluatePBR(in PBRInput input)
{ 
    m_pbrInput = input;

    const float3 dirToCamera = normalize(View.cameraPosition.xyz - m_pbrInput.worldPosition);
    
    float3 lightOutput = 0.f;
     
    BRDFInput brdfInput; 
    brdfInput.V = dirToCamera;
    brdfInput.N = m_pbrInput.normal;
    brdfInput.diffuseColor = CalculateDiffuseColor(m_pbrInput.albedo.rgb, m_pbrInput.metallic);
    brdfInput.f0 = CalculateF0(m_pbrInput.albedo.rgb, m_pbrInput.metallic);
    brdfInput.f90 = CalculateF90(m_pbrInput.albedo.rgb, m_pbrInput.metallic);
    brdfInput.roughness = m_pbrInput.roughness;
    brdfInput.metalness = m_pbrInput.metallic;

    lightOutput += EvaluateLights(dirToCamera, View.lightCount);

    const float3 compositeLighting = lightOutput + m_pbrInput.emissive;
    return compositeLighting;
}