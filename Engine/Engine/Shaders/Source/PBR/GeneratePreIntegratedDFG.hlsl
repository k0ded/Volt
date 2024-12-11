#include "Vertex.hlsli"
#include "BRDF.hlsli"

uint2 GetHammerslyPoint(uint num, uint bitCount)
{
    uint rev = reversebits(num << (32 - bitCount));
    return uint2(num, rev);
}

float2 GetHammerslyPointFloat(uint num, uint bitCount)
{
    uint2 pt = GetHammerslyPoint(num, bitCount);
    float2 floatPt = (float2)pt * float(1.f / (1u << bitCount));

    return floatPt;
}

// Evaluate the normal distribution term for GGX.
float D_GGX(float NdotH, float roughness, float normValue)
{
	float a = roughness * roughness;
	float a2 = a * a;
	//float d = ((NdotH * a2) - NdotH) * NdotH + 1;
	float d = ((NdotH*NdotH) * (a2-1)) + 1;
	//d = max(d, 1e-7f);
	return (a2 * normValue) / (PI * (d * d));
}

// Importance sample the BRDF for GGX at the specified sample point. Also calculate the geometric term G.
void ImportanceSampleGGX_G(
    float2 u, float3 V, float3 N, float3 tangentX, float3 tangentY, float roughness, 
    out float NdotH, out float LdotH, out float3 L, out float3 H, out float G)
{
    float a = roughness * roughness;
    
    float phi = 2.f * PI * u.x;
    float sinPhi, cosPhi;
    sincos(phi, sinPhi, cosPhi);

    float cosTheta2 = (1.f - u.y) / (1.f + (a * a - 1.f) * u.y);
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.f - min(1.f, cosTheta2));

    float3 HTangentSpace = float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
    
    H = tangentX * HTangentSpace.x + tangentY * HTangentSpace.y + N * HTangentSpace.z;

    L = (2.f * dot(V, H) * H) - V;

    NdotH = saturate(dot(N, H));
    LdotH = saturate(dot(L, H));
    
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    G = V_SmithGGXCorrelated(NdotL, NdotV, roughness);
}

// Importance sample the BRDF for GGX at the specified sample point. Also calculate the geometric term G.
float ImportanceSampleGGX_D(float2 u, float3 N, float3 tangentX, float3 tangentY, float roughness)
{
    float a = roughness * roughness;
    
    float phi = 2.f * PI * u.x;
    float sinPhi, cosPhi;
    sincos(phi, sinPhi, cosPhi);
    float cosTheta2 = (1.f - u.y) / (1.f + (a * a - 1.f) * u.y);
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.f - min(1.f, cosTheta2));

    float3 HTangentSpace = float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
    
    float3 H = tangentX * HTangentSpace.x + tangentY * HTangentSpace.y + N * HTangentSpace.z;
    float NdotH = saturate(dot(N, H));
    float D = D_GGX(NdotH, roughness, 1.f);
    
    return D;
}

void ImportanceSampleCosDir(
    float2 u, float3 N, float3 tangentX, float3 tangentY,
    out float3 L, out float NdotL, out float pdf)
{
    float u1 = u.x;
    float u2 = u.y;

    float r = sqrt(u1);
    float h = sqrt(max(0.f, 1.f - u1));
    float phi = u2 * PI * 2.f;
    float cosPhi, sinPhi;
    sincos(phi, sinPhi, cosPhi);

    // Generate ray in surface hemisphere.
    L = float3(r * cosPhi, r * sinPhi, h);

    L = normalize(tangentX * L.x + tangentY * L.y + N * L.z);

    NdotL = dot(L, N);
    pdf = NdotL / PI;
}

float4 IntegrateDFGOnly(float3 V, float roughness)
{
    roughness = max(roughness, 0.001f);
    
    const float3 up = float3(0.f, 1.f, 0.f);
    const float3 N = float3(0.f, 0.f, 1.f);
    const float3 tangentX = normalize(cross(up, N));
    const float3 tangentY = cross(N, tangentX);

    const float linearRoughness = sqrt(roughness);

    const float NdotV = saturate(dot(N, V));
    float4 accumulated = 0.f;

    uint sampleBitCount = 10;
    uint sampleCount = 1u << sampleBitCount;

    for (uint i = 0; i < sampleCount; i++)
    {
        float2 u = GetHammerslyPointFloat(i, sampleBitCount);
        float3 L = 0.f;
        float3 H = 0.f;
        float NdotH = 0.f;
        float LdotH = 0.f;
        float G = 0.f;

        ImportanceSampleGGX_G(u, V, N, tangentX, tangentY, roughness, NdotH, LdotH, L, H, G);

		// Specular GGX DFG preintegration into acc.xy
        float NdotL = dot(N, L);
        if (NdotL > 0.f)
        {
            float VdotH = dot(V, H);
            float GVis = NdotL * G * (4.f * VdotH / NdotH);
            float Fc = pow(1.f - VdotH, 5.f);
            
            accumulated.x += (1.f - Fc) * GVis;
            accumulated.y += Fc * GVis;
        }

        // Diffuse Disney preintegration into acc.z
        u = frac(u + 0.5f);
        float pdf = 0.f;
        ImportanceSampleCosDir(u, N, tangentX, tangentY, L, NdotL, pdf);
        if (NdotL > 0.f)
        {
            float LdotH2 = saturate(dot(L, normalize(V + L)));
            accumulated.z += Fr_DisneyDiffuse(NdotV, NdotL, LdotH2, linearRoughness, 1.f).x;
        }

        accumulated.w += ImportanceSampleGGX_D(u, N, tangentX, tangentY, roughness);
    }

    accumulated /= float(sampleCount);
    accumulated.w = 1.f / accumulated.w;

    return accumulated;
}

struct Output
{
    [[vt::rgba16f]] float4 color : SV_Target0;
};

Output MainPS(FullscreenTriangleVertex input)
{
    float2 uv = input.uv;

    float cosTheta = uv.x;
    float sinTheta = sqrt(1.f - (cosTheta * cosTheta));

    float3 V = float3(0.f, sinTheta, cosTheta);
    float roughness = uv.y;

    Output output;
    output.color = IntegrateDFGOnly(V, roughness);
    return output;
}