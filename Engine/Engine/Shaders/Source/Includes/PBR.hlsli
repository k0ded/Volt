#pragma once

#include "Structures.hlsli"
#include "Resources.hlsli"
#include "ShadowMapping.hlsli"
#include "Lights.hlsli"
#include "RayTracing.hlsli"
#include "Exposure/Exposure.hlsli"

#include "PBR/LightEvaluation.hlsli"

struct PBRConstants
{
    vt::UniformBuffer<ViewData> viewData;
    
    vt::UniformBuffer<DirectionalLight> directionalLight;
    vt::TypedBuffer<PointLight> pointLights;
    vt::TypedBuffer<SpotLight> spotLights;

    vt::TypedBuffer<int> visiblePointLights;
    vt::TypedBuffer<int> visibleSpotLights;
    
    vt::TextureSampler linearSampler;
    vt::TextureSampler pointLinearClampSampler;
    vt::TextureSampler shadowSampler;
     
    vt::Tex2D<float4> DFGLuT;
    vt::Tex2DArray<float> directionalShadowMap;

    SkyLight skyLight;
};

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
static PBRConstants m_pbrConstants;
static ViewData m_viewData;

float3 CalculatePointLights(float3 dirToCamera, uint pointLightCount)
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

    for (int i = 0; i < pointLightCount; i++)
    {
        int lightIndex = GetLightBufferIndex(m_pbrConstants.visiblePointLights, m_viewData.tileCountX, i, m_pbrInput.tileId);
        if (lightIndex == -1)
        { 
            break; 
        }

        output += CalculatePointLight(m_pbrConstants.pointLights.Load(i), brdfInput, m_pbrInput.worldPosition);
    }
    
    return output;
}

float3 CalculateSpotLights(float3 dirToCamera, uint spotLightCount)
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

    for (uint i = 0; i < spotLightCount; i++)
    {
        output += CalculateSpotLight(m_pbrConstants.spotLights.Load(i), brdfInput, m_pbrInput.worldPosition);
    }
    
    return output; 
}

float3 CalculatePBR(in PBRInput input, in PBRConstants constants)
{ 
    m_pbrInput = input;
    m_pbrConstants = constants;
    
    m_viewData = constants.viewData.Load();

    const float3 dirToCamera = normalize(m_viewData.cameraPosition.xyz - m_pbrInput.worldPosition);
    
    float3 lightOutput = 0.f;
     
    BRDFInput brdfInput; 
    brdfInput.V = dirToCamera;
    brdfInput.N = m_pbrInput.normal;
    brdfInput.diffuseColor = CalculateDiffuseColor(m_pbrInput.albedo.rgb, m_pbrInput.metallic);
    brdfInput.f0 = CalculateF0(m_pbrInput.albedo.rgb, m_pbrInput.metallic);
    brdfInput.f90 = CalculateF90(m_pbrInput.albedo.rgb, m_pbrInput.metallic);
    brdfInput.roughness = m_pbrInput.roughness;
    brdfInput.metalness = m_pbrInput.metallic;

    // Skylight
    {
        //lightOutput += CalculateIBL(brdfInput, constants.DFGLuT, constants.linearSampler, constants.skyLight) * input.ao; 
    }
    
    // Directional Light
    {
        DirectionalShadowMappingInfo shadowMappingInfo;
        shadowMappingInfo.shadowMap = m_pbrConstants.directionalShadowMap;
        shadowMappingInfo.shadowSampler = m_pbrConstants.shadowSampler;
        shadowMappingInfo.viewMatrix = m_viewData.view;

        lightOutput += CalculateDirectionalLight(constants.directionalLight.Load(), shadowMappingInfo, brdfInput, m_pbrInput.worldPosition);
    }
    
    // Point lights
    {
        lightOutput += CalculatePointLights(dirToCamera, m_viewData.pointLightCount);
    }
    
    // Spot lights
    {
        lightOutput += CalculateSpotLights(dirToCamera, m_viewData.spotLightCount);
    }
    
    const float3 compositeLighting = lightOutput + m_pbrInput.emissive;
    return compositeLighting;
}