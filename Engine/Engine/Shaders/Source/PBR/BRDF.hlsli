#pragma once

#include "MathConstants.hlsli"

struct BRDFParameters
{
    float NdotL;
    float NdotV;
    float NdotH;
    float LdotH;
    float LdotV;
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

float3 LambertDiffuse(float3 diffuse)
{
    return diffuse / PI;
}

float3 Fr_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness, float3 diffuseColor)
{
    float energyBias = lerp(0.f, 0.5f, linearRoughness);
    float energyFactor = lerp(1.f, 1.f / 1.51f, linearRoughness);
    float Fd90 = energyBias * 2.f * LdotH * LdotH * linearRoughness;
    float3 F0 = 1.f;

    float lightScatter = F_Schlick(F0, Fd90, NdotL).r;
    float viewScatter = F_Schlick(F0, Fd90, NdotV).r;

    return diffuseColor * ( lightScatter * viewScatter * energyFactor / PI);
}

float3 OrenNayarDiffuse(float LdotV, float NdotL, float NdotV, float alphaRoughness, float3 diffuseColor)
{
    float s = LdotV - NdotL * NdotV;
    float t = lerp(1.f, max(NdotL, NdotV), step(0.f, s));

    float sigma2 = alphaRoughness;
    float A = 1.f + sigma2 * (1.f / (sigma2 + 0.13f) + 0.5f / (sigma2 + 0.33f));
    float B = 0.45f * sigma2 / (sigma2 + 0.09f);

    return diffuseColor * max(0.f, NdotL) * (A + B * s / t) / PI;
}

float3 DiffuseBRDF(BRDFParameters pbrInput)
{
    return OrenNayarDiffuse(pbrInput.LdotV, pbrInput.NdotL, pbrInput.NdotV, pbrInput.alphaRoughness, pbrInput.diffuseColor);
    //return  Fr_DisneyDiffuse(pbrInput.NdotV, pbrInput.NdotL, pbrInput.LdotH, pbrInput.roughness, pbrInput.diffuseColor);
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
    float LdotV = saturate(dot(L, input.V));

    input.roughness = max(input.roughness, 0.05f);

    BRDFParameters pbrInput;
    pbrInput.NdotL = NdotL;
    pbrInput.NdotV = NdotV;
    pbrInput.NdotH = NdotH;
    pbrInput.LdotH = LdotH;
    pbrInput.LdotV = LdotV;
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

    input.roughness = max(input.roughness, 0.05f);

    // Diffuse
    {
        float3 H = normalize(input.V + D);
        float LdotH = saturate(dot(D, H));
        float NdotH = saturate(dot(input.N, H));
        float NdotL = saturate(dot(input.N, D));
        float LdotV = saturate(dot(D, input.V));

        BRDFParameters brdfParams;
        brdfParams.NdotL = NdotL;
        brdfParams.NdotV = NdotV;
        brdfParams.NdotH = NdotH;
        brdfParams.LdotH = LdotH;
        brdfParams.LdotV = LdotV;
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
        float LdotV = saturate(dot(L, input.V));

        BRDFParameters brdfParams;
        brdfParams.NdotL = NdotL;
        brdfParams.NdotV = NdotV;
        brdfParams.NdotH = NdotH;
        brdfParams.LdotH = LdotH;
        brdfParams.LdotV = LdotV;
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