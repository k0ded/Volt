#include "Vertex.hlsli"

static const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i)/float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	float3 up          = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent   = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);
	
	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    // note that we use a different k for IBL
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float2 IntegrateBRDF(float NdotV, float roughness)
{
    float3 V = float3(sqrt(1.f - NdotV * NdotV), 0.f, NdotV);

    float A = 0.f;
    float B = 0.f;

    float3 N = float3(0.f, 0.f, 1.f);

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0; i < SAMPLE_COUNT; i++)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.f * dot(V, H) * H - V);

		float NdotL = max(L.z, 0.f);
		float NdotH = max(H.z, 0.f);
		float VdotH = max(dot(V, H), 0.f);

		if (NdotL > 0.f)
		{
			float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.f - VdotH, 5.f);

            A += (1.f - Fc) * G_Vis;
            B += Fc * G_Vis;
		}
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    return float2(A, B);
}

uint2 GetHammerslyPoint(uint num, uint bitCount)
{
    uint rev = reversebits(num << 32 - bitCount);
    return uint2(num, rev);
}

float2 GetHammerslyPointFloat(uint num, uint bitCount)
{
    uint2 pt = GetHammerslyPoint(num, bitCount);
    float2 floatPt = (float2)pt * float(1.f / (1 << bitCount));

    return floatPt;
}

// #TODO_Ivar: Include from BRDF.hlsli instead
float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
    float alphaG2 = alphaG * alphaG;
    float Lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
    float Lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

    return 0.5f / max((Lambda_GGXV + Lambda_GGXL), 0.0001f);
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
    uint sampleCount = 1 << sampleBitCount;

    for (uint i = 0; i < sampleCount; i++)
    {
        float2 u = GetHammerslyPointFloat(i, sampleBitCount);
        float3 L = 0.f;
        float3 H = 0.f;
        float NdotH = 0.f;
        float LdotH = 0.f;
        float G = 0.f;

        ImportanceSampleGGX_G(u, V, N, tangentX, tangentY, roughness, NdotH, LdotH, L, H, G);

        float NdotL = dot(N, L);
        if (NdotL > 0.f)
        {
            
        }
    }
}

struct Output
{
    [[vt::rgba16f]] float4 color : SV_Target0;
};

Output MainPS(FullscreenTriangleVertex input)
{
    Output output;
    output.color = IntegrateBRDF(input.uv.x, input.uv.y);
    return output;
}