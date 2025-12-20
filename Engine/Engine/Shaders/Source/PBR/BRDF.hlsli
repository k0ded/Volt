#pragma once

#include "MathConstants.hlsli"

struct BRDFInput
{
    float3 V;
    float3 N;

    float3 baseColor;
    float roughness;
    float metalness;
};

static const float3 DielectricF0 = float3(0.04f, 0.04f, 0.04f);

float D_GGX(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float d = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * d * d);
}

float V_SmithGGXCorrelated(float NdotV, float NdotL, float alpha)
{
    float a2 = alpha * alpha;
    float gv = NdotL * sqrt(NdotV * (NdotV - a2 * NdotV) + a2);
    float gl = NdotV * sqrt(NdotL * (NdotL - a2 * NdotL) + a2);
    return 0.5f / max(gv + gl, 1e-6f);
}

float3 F_Schlick(float3 F0, float cosTheta)
{
    float f = pow(1.0f - cosTheta, 5.0f);
    return F0 + (1.0f - F0) * f;
}

float3 Diffuse_Lambert(float3 diffuseColor)
{
    return diffuseColor / PI;
}

float3 Diffuse_Disney(float3 diffuseColor, float roughness, float NdotV, float NdotL, float LdotH)
{
    float fd90 = 0.5f + 2.0f * roughness * LdotH * LdotH;
    float lightScatter = 1.0f + (fd90 - 1.0f) * pow(1.0f - NdotL, 5.0f);
    float viewScatter  = 1.0f + (fd90 - 1.0f) * pow(1.0f - NdotV, 5.0f);
    return diffuseColor * lightScatter * viewScatter / PI;
}

float3 Diffuse_OrenNayar(float3 diffuseColor, float roughness, float NdotV, float NdotL, float LdotV)
{
    float sigma = roughness * PI * 0.5f;
    float sigma2 = sigma * sigma;

    float s = LdotV - NdotL * NdotV;
    float t = (s > 0.0f) ? max(NdotL, NdotV) : 1.0f;

    float A = 1.0f - 0.5f * sigma2 / (sigma2 + 0.33f);
    float B = 0.45f * sigma2 / (sigma2 + 0.09f);

    return diffuseColor * max(0.0f, NdotL) * (A + B * s / t) / PI;
}

float3 BRDF_Lambert(BRDFInput input, float3 L)
{
    float3 N = input.N;
    float3 V = input.V;

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    if (NdotL <= 0.0f || NdotV <= 0.0f)
    {
        return 0.0f;
    }

    float3 H = normalize(V + L);

    float NdotH = saturate(dot(N, H));
    float LdotH = saturate(dot(L, H));

    float roughness = max(input.roughness, 0.045f);
    float alpha = roughness * roughness;

    float3 F0 = lerp(DielectricF0, input.baseColor, input.metalness);
    float3 F = F_Schlick(F0, LdotH);

    float  D = D_GGX(NdotH, alpha);
    float  Vg = V_SmithGGXCorrelated(NdotV, NdotL, alpha);
    float3 Fr = D * Vg * F;

    float3 kd = (1.0f - F) * (1.0f - input.metalness);
    float3 diffuseColor = input.baseColor * kd;
    float3 Fd = Diffuse_Lambert(diffuseColor);

    return (Fd + Fr) * NdotL;
}

float3 BRDF_DisneyDiffuse(BRDFInput input, float3 L)
{
    float3 N = input.N;
    float3 V = input.V;

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    if (NdotL <= 0.0f || NdotV <= 0.0f)
    {
        return 0.0f;
    }

    float3 H = normalize(V + L);

    float NdotH = saturate(dot(N, H));
    float LdotH = saturate(dot(L, H));
    float LdotV = saturate(dot(L, V));

    float roughness = max(input.roughness, 0.045f);
    float alpha = roughness * roughness;

    float3 F0 = lerp(DielectricF0, input.baseColor, input.metalness);
    float3 F = F_Schlick(F0, LdotH);

    float  D = D_GGX(NdotH, alpha);
    float  Vg = V_SmithGGXCorrelated(NdotV, NdotL, alpha);
    float3 Fr = D * Vg * F;

    float3 kd = (1.0f - F) * (1.0f - input.metalness);
    float3 diffuseColor = input.baseColor * kd;
    float3 Fd = Diffuse_Disney(diffuseColor, roughness, NdotV, NdotL, LdotH);

    return (Fd + Fr) * NdotL;
}

float3 BRDF_OrenNayar(BRDFInput input, float3 L)
{
    float3 N = input.N;
    float3 V = input.V;

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    if (NdotL <= 0.0f || NdotV <= 0.0f)
    {
        return 0.0f;
    }

    float3 H = normalize(V + L);

    float NdotH = saturate(dot(N, H));
    float LdotH = saturate(dot(L, H));
    float LdotV = saturate(dot(L, V));

    float roughness = max(input.roughness, 0.045f);
    float alpha = roughness * roughness;

    float3 F0 = lerp(DielectricF0, input.baseColor, input.metalness);
    float3 F = F_Schlick(F0, LdotH);

    float  D = D_GGX(NdotH, alpha);
    float  Vg = V_SmithGGXCorrelated(NdotV, NdotL, alpha);
    float3 Fr = D * Vg * F;

    float3 kd = (1.0f - F) * (1.0f - input.metalness);
    float3 diffuseColor = input.baseColor * kd;
    float3 Fd = Diffuse_OrenNayar(diffuseColor, roughness, NdotV, NdotL, LdotV);

    return (Fd + Fr) * NdotL;
}
