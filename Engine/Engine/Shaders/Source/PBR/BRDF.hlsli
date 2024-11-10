#pragma once

#include "Lights.hlsli"
#include "ShadowMapping.hlsli"

struct BRDFParameters
{
    float NdotL;
    float NdotV;
    float NdotH;
    float LdotH;
    float roughness;
    float metalness;
    float3 f0;
    float3 f90;
    float alphaRoughness;
    float3 diffuseColor;
    float3 specularColor;
};

struct BRDFInput
{
    float3 V;
    float3 N;

    float3 diffuseColor;
    float3 f0;
    float3 f90;
    float roughness;
    float metalness;
};

static const float3 m_F0 = 0.04f;

float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
    float alphaG2 = alphaG * alphaG;
    float Lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
    float Lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

    return 0.5f / max((Lambda_GGXV + Lambda_GGXL), 0.0001f);
}

float D_GGX(float NdotH, float m)
{
    float m2 = m * m;
    float f = (NdotH * m2 - NdotH) * NdotH + 1;
    return m2 / (f * f);
}

float3 F_Schlick(float3 F0, float3 F90, float u)
{
    return F0 + (F90 - F0) * pow(1.f - u, 5.f);
}

float Frostbite_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float energyBias = lerp(0.f, 0.5f, linearRoughness);
    float energyFactor = lerp(1.f, 1.f / 1.51f, linearRoughness);
    float Fd90 = energyBias * 2.f * LdotH * LdotH * linearRoughness;
    float3 F0 = 1.f;

    float lightScatter = F_Schlick(F0, Fd90, NdotL).r;
    float viewScatter = F_Schlick(F0, Fd90, NdotV).r;

    return lightScatter * viewScatter * energyFactor;
}

float3 DiffuseBRDF(BRDFParameters pbrInput)
{
    float Fd = Frostbite_DisneyDiffuse(pbrInput.NdotV, pbrInput.NdotL, pbrInput.LdotH, pbrInput.roughness) / PI;
    return Fd * pbrInput.diffuseColor;
}

float3 SpecularBRDF(BRDFParameters pbrInput)
{
    float3 F = F_Schlick(pbrInput.f0, pbrInput.f90, pbrInput.LdotH);
    float Vis = V_SmithGGXCorrelated(pbrInput.NdotV, pbrInput.NdotL, pbrInput.alphaRoughness);
    float D = D_GGX(pbrInput.NdotH, pbrInput.alphaRoughness);
    float3 Fr = D * F * Vis / PI;

    return Fr;
}

float3 CalculateF0(float3 baseColor, float metallic)
{
    float3 F0 = m_F0;
    float3 specularColor = lerp(F0, baseColor, metallic);

    return specularColor;
}

float3 CalculateF90(float3 baseColor, float metallic)
{
    float3 F0 = m_F0;
    float3 specularColor = lerp(F0, baseColor, metallic);

    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    float reflectance90 = clamp(reflectance * 25.f, 0.f, 1.f);

    return reflectance90;
}

float3 CalculateDiffuseColor(float3 baseColor, float metallic)
{
    float3 diffuseColor = baseColor * (1.f - m_F0);
    diffuseColor *= 1.f - metallic;

    return diffuseColor;
}

float3 BRDF(BRDFInput input, float3 L)
{
    float NdotV = abs(dot(input.N, input.V)) + 1e-5f;
    float3 H = normalize(input.V + L);
    float LdotH = saturate(dot(L, H));
    float NdotH = saturate(dot(input.N, H));
    float NdotL = saturate(dot(input.N, L));

    input.roughness = max(input.roughness, 0.05f);

    BRDFParameters pbrInput;
    pbrInput.NdotL = NdotL;
    pbrInput.NdotV = NdotV;
    pbrInput.NdotH = NdotH;
    pbrInput.LdotH = LdotH;
    pbrInput.roughness = input.roughness;
    pbrInput.alphaRoughness = input.roughness * input.roughness;
    pbrInput.metalness = input.metalness;
    pbrInput.f0 = input.f0;
    pbrInput.f90 = input.f90;
    pbrInput.diffuseColor = input.diffuseColor;
    pbrInput.specularColor = pbrInput.f0;

    float3 Fd = DiffuseBRDF(pbrInput);
    float3 Fr = SpecularBRDF(pbrInput);

    // BRDF scaled by energy of the light (cosine law)
    return NdotL * (Fd + Fr);
}

float3 BRDF(BRDFInput input, float3 D, float3 L)
{
    float NdotV = abs(dot(input.N, input.V)) + 1e-5f;

    float3 Fd = 0.f;
    float3 Fr = 0.f;

    // Diffuse
    {
        float3 H = normalize(input.V + D);
        float LdotH = saturate(dot(D, H));
        float NdotH = saturate(dot(input.N, H));
        float NdotL = saturate(dot(input.N, D));

        BRDFParameters brdfParams;
        brdfParams.NdotL = NdotL;
        brdfParams.NdotV = NdotV;
        brdfParams.NdotH = NdotH;
        brdfParams.LdotH = LdotH;
        brdfParams.roughness = input.roughness;
        brdfParams.alphaRoughness = input.roughness * input.roughness;
        brdfParams.metalness = input.metalness;
        brdfParams.f0 = input.f0;
        brdfParams.f90 = input.f90;
        brdfParams.diffuseColor = input.diffuseColor;
        brdfParams.specularColor = brdfParams.f0;

        Fd = DiffuseBRDF(brdfParams);
    }

    // Specular
    {
        float3 H = normalize(input.V + L);
        float LdotH = saturate(dot(L, H));
        float NdotH = saturate(dot(input.N, H));
        float NdotL = saturate(dot(input.N, L));

        BRDFParameters brdfParams;
        brdfParams.NdotL = NdotL;
        brdfParams.NdotV = NdotV;
        brdfParams.NdotH = NdotH;
        brdfParams.LdotH = LdotH;
        brdfParams.roughness = input.roughness;
        brdfParams.alphaRoughness = input.roughness * input.roughness;
        brdfParams.metalness = input.metalness;
        brdfParams.f0 = input.f0;
        brdfParams.f90 = input.f90;
        brdfParams.diffuseColor = input.diffuseColor;
        brdfParams.specularColor = brdfParams.f0;

        Fr = SpecularBRDF(brdfParams);
    }

    return Fd + Fr;
}

float SmoothDistanceAttenuation(float squaredDistance, float invSqrAttRadius)
{
    float factor = squaredDistance * invSqrAttRadius;
    float smoothFactor = saturate(1.f - factor * factor);
    return smoothFactor * smoothFactor;
}

float GetDistanceAttenuation(float3 unormalizedLightVector, float invSqrAttRadius)
{
    float sqrDist = dot(unormalizedLightVector, unormalizedLightVector);
    float attenuation = 1.f / max(sqrDist, 1.f);
    attenuation *= SmoothDistanceAttenuation(sqrDist, invSqrAttRadius);

    return attenuation;
}

float GetAngleAttenuation(float3 normalizedLightVector, float3 lightDirection, float lightAngleScale, float lightAngleOffset)
{
    float cd = dot(lightDirection, normalizedLightVector);
    float attenuation = saturate(cd * lightAngleScale + lightAngleOffset);
    attenuation *= attenuation;

    return attenuation;
}

float3 CalculatePointLight2(in PointLight light, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 unormalizedLightVector = light.position - worldPosition;
    float3 L = normalize(unormalizedLightVector);
    float invSqrRadius = 1.f / (light.radius * light.radius);

    float attenuation = GetDistanceAttenuation(unormalizedLightVector, invSqrRadius);

    return BRDF(brdfInput, L) * light.color * light.intensity * attenuation;   
}

float3 CalculateSpotLight2(in SpotLight light, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 unormalizedLightVector = light.position - worldPosition;
    float3 L = normalize(unormalizedLightVector);
    float invSqrRadius = 1.f / (light.range * light.range);
    
    float attenuation = 1.f;
    attenuation *= GetDistanceAttenuation(unormalizedLightVector, invSqrRadius);
    attenuation *= GetAngleAttenuation(L, normalize(light.direction), light.lightAngleScale, light.lightAngleOffset);
    
    return BRDF(brdfInput, L) * light.color * attenuation * light.intensity;   
} 

float CalculateDirectionalShadow2(in DirectionalLight light)
{
    //const uint cascadeIndex = GetCascadeIndexFromWorldPosition(light, m_pbrInput.worldPosition, m_viewData.view);
    //const float3 shadowMapCoords = GetShadowMapCoords(light.viewProjections[cascadeIndex], m_pbrInput.worldPosition);
    //const float result = CalculateDirectionalShadow_Hard(light, m_shadowSampler, m_pbrConstants.directionalShadowMap, m_pbrInput.normal, cascadeIndex, shadowMapCoords);
    return 1;
} 

float3 CalculateDirectionalLight2(in DirectionalLight light, in BRDFInput brdfInput, float3 worldPosition)
{
    float3 D = normalize(light.direction.xyz);
    float r = sin(light.angularRadius);
    float d = cos(light.angularRadius);

    float DdotV = dot(D, brdfInput.V);
    float3 S = brdfInput.V - DdotV * D;
    float3 L = DdotV < d ? normalize(d * D * normalize(S) * r) : brdfInput.V;

    float illuminance = light.intensity * saturate(dot(brdfInput.N, D));

    return BRDF(brdfInput, D, L) * light.color * illuminance;
} 

float3 GetSpecularDominantDirection(float3 N, float3 R, float roughness)
{
    float smoothness = saturate(1.f - roughness);
    float lerpFactor = smoothness * (sqrt(smoothness) + roughness);

    return lerp(N, R, lerpFactor);
}

float3 GetDiffuseDominantDirection(float3 N, float3 V, float NdotV, float roughness)
{
    float a = 1.02341f * roughness - 1.51174f;
    float b = -0.511705f * roughness + 0.755868f;
    
    float lerpFactor = saturate((NdotV * a + b) * roughness);
    return lerp(N, V, lerpFactor);
}

float3 CalculateSkyAmbiance2(in BRDFInput brdfInput, vt::TexCube<float3> irradiance, vt::TexCube<float3> radiance, TextureSampler linearSampler)
{
    float NdotV = saturate(dot(brdfInput.N, brdfInput.V));
    float    

    float3 diffuse = 0.f;

    {
        float3 dominantN = GetDiffuseDominantDirection(brdfInput.N, brdfInput.V, NdotV, brdfInput.roughness);
        float3 diffuseLighting = irradiance.SampleLevel(linearSampler, dominantN, 0.f);

            
    }
}