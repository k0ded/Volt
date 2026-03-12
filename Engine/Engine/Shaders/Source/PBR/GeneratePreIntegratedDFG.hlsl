#include "Utility/FullscreenTriangleVertex.hlsli"
#include "MathConstants.hlsli"
#include "BRDF.hlsli"

uint2 Hammersley(uint i, uint bits)
{
    uint r = reversebits(i) >> (32 - bits);
    return uint2(i, r);
}

float2 HammersleyFloat(uint i, uint bits)
{
    return float2(Hammersley(i, bits)) / float(1u << bits);
}

void ImportanceSampleGGX(
    float2 u,
    float3 V,
    float3 N,
    float3 T,
    float3 B,
    float roughness,
    out float3 L,
    out float3 H,
    out float NdotL,
    out float NdotH,
    out float VdotH,
    out float Vg
)
{
    float alpha = roughness * roughness;

    float phi = 2.0f * PI * u.x;
    float cosPhi, sinPhi;
    sincos(phi, sinPhi, cosPhi);

    float cosTheta2 = (1.0f - u.y) / (1.0f + (alpha * alpha - 1.0f) * u.y);
    float cosTheta  = sqrt(cosTheta2);
    float sinTheta  = sqrt(1.0f - cosTheta2);

    float3 Ht = float3(
        sinTheta * cosPhi,
        sinTheta * sinPhi,
        cosTheta
    );

    H = normalize(T * Ht.x + B * Ht.y + N * Ht.z);
    L = reflect(-V, H);

    NdotL = saturate(dot(N, L));
    NdotH = saturate(dot(N, H));
    VdotH = saturate(dot(V, H));

    float NdotV = saturate(dot(N, V));
    Vg = V_SmithGGXCorrelated(NdotV, NdotL, alpha);
}

float2 IntegrateDFG(float3 V, float roughness)
{
    roughness = max(roughness, 0.045f);

    const float3 N = float3(0, 0, 1);
    const float3 up = float3(0, 1, 0);
    const float3 T = normalize(cross(up, N));
    const float3 B = cross(N, T);

    float NdotV = saturate(dot(N, V));

    float2 acc = 0.0f;

    const uint SAMPLE_BITS  = 10;
    const uint SAMPLE_COUNT = 1u << SAMPLE_BITS;

    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float2 u = HammersleyFloat(i, SAMPLE_BITS);

        float3 L, H;
        float NdotL, NdotH, VdotH, Vg;

        ImportanceSampleGGX(
            u, V, N, T, B, roughness,
            L, H, NdotL, NdotH, VdotH, Vg
        );

        if (NdotL > 0.0f)
        {
            float GVis = Vg * NdotL / max(NdotH, 1e-5f);
            float Fc = pow(1.0f - VdotH, 5.0f);

            acc.x += (1.0f - Fc) * GVis;
            acc.y += Fc * GVis;
        }
    }

    return acc / float(SAMPLE_COUNT);
}

float2 MainPS(FullscreenTriangleVertex input) : SV_Target0
{
    float2 uv = input.uv;

    float NdotV = saturate(uv.x);
    float sinTheta = sqrt(1.0f - NdotV * NdotV);
    float3 V = float3(0.0f, sinTheta, NdotV);

    float roughness = uv.y;

    float2 dfg = IntegrateDFG(V, roughness);
    return dfg;
}
