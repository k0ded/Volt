#pragma once

static const float PI = 3.14159265359f;
static const float m_minRoughness = 0.04f;

float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
    float alphaG2 = alphaG * alphaG;
    float Lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
    float Lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

    return 0.5f / (Lambda_GGXV + Lambda_GGXL);
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

float DiffuseBRDF(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float Fd = Frostbite_DisneyDiffuse(NdotV, NdotL, LdotH, linearRoughness) / PI;
    return Fd;
}

float SpecularBRDF(float NdotV, float NdotL, float LdotH, float NdotH, float roughness, float3 F0, float F90)
{
    float3 F = F_Schlick(F0, F90, LdotH);
    float Vis = V_SmithGGXCorrelated(NdotV, NdotL, roughness);
    float D = D_GGX(NdotH, roughness);
    float Fr = D * F * Vis / PI;

    return Fr;
}

float BRDF(float3 V, float3 N, float3 L, float3 F0, float F90, float roughness)
{
    float NdotV = abs(dot(N, V)) + 1e-5f;
    float3 H = normalize(V + L);
    float LdotH = saturate(dot(L, H));
    float NdotH = saturate(dot(N, H));
    float NdotL = saturate(dot(N, L));

    roughness = clamp(roughness, m_minRoughness, 1.f);

    float alphaRoughness = roughness * roughness;

    float Fd = DiffuseBRDF(NdotV, NdotL, LdotH, roughness);
    float Fr = SpecularBRDF(NdotV, NdotL, LdotH, NdotH, alphaRoughness, F0, F90);

    return Fd + Fr;
}

