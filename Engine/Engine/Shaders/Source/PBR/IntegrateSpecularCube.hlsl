RWTexture2DArray<float3> RWOutput;
TextureCube<float3> Input;
SamplerState LinearSampler;

uint MipIndex;
uint MipCount;

static const float PI = 3.14159265359;

float2 GetSample(uint index, uint numSamples, uint2 random)
{
	float E1 = frac( (float)index / numSamples + float( random.x & 0xffff ) / (1<<16) );
	float E2 = float( reversebits(index) ^ random.y ) * 2.3283064365386963e-10;
	return float2( E1, E2 );
}

void ImportanceSampleGGX_Dir(float2 u, float3 V, float3 N, float roughness, out float3 H, out float3 L)
{
    float3 upVector = abs(N.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
    float3 tangentX = normalize(cross(upVector, N));
    float3 tangentY = cross(N, tangentX);

    float a = roughness * roughness;
    float a2 = a * a;

    // GGX NDF sampling
    float cosThetaH = sqrt((1.f - u.x) / (1.f + (roughness * roughness - 1.f) * u.x));
    float sinThetaH = sqrt(1.f - min(1.f, cosThetaH * cosThetaH));
    float phiH = u.y * PI * 2.f;

    H = float3(sinThetaH * cos(phiH), sinThetaH * sin(phiH), cosThetaH);
    H = normalize(tangentX * H.y + tangentY * H.x + N * H.z);
    L = 2.f * dot(V, H) * H - V;
}

float D_GGX_Divide_Pi(float NdotH, float m)
{
	float m2 = m * m;
	float f = (NdotH * m2 - NdotH) * NdotH + 1;
	return (m2 / (PI * f * f));
}

float3 IntegrateCubeLDOnly(float3 V, float3 N, float roughness)
{
    const uint sampleCount = 32;
    const uint cubeSize = 1u << (MipCount - 1);

    float3 accBrdf = 0.f;
    float accBrdfWeight = 0.f;

    for (uint i = 0; i < sampleCount; i++)
    {
        float2 eta = GetSample(i, sampleCount, 0);
        float3 L;
        float3 H;
        
        ImportanceSampleGGX_Dir(eta, V, N, roughness, H, L);
        float NdotL = dot(N, L);
        if (NdotL > 0.f)
        {
            float NdotH = saturate(dot(N, H));
            float LdotH = saturate(dot(L, H));
            float pdf = D_GGX_Divide_Pi(NdotH, roughness) * NdotH / (4.f * LdotH);
            float omegaS = 1.f / (sampleCount * pdf);
            float omegaP = 4.f * PI / (6.f * cubeSize * cubeSize) * 2.f;
            float mipLevel = 0.5f * log2(omegaS / omegaP);

            float3 Li = Input.SampleLevel(LinearSampler, L, mipLevel);

            accBrdf += Li * NdotL;
            accBrdfWeight += NdotL;
        }
    }

    return accBrdf * (1.f / accBrdfWeight);
}

float3 GetCubeMapTexCoord(uint3 dispatchId)
{
    uint2 texSize;
    uint elements;

    RWOutput.GetDimensions(texSize.x, texSize.y, elements);

    float2 ST = dispatchId.xy / float2(texSize.x, texSize.y);
    float2 UV = 2.f * float2(ST.x, 1.f - ST.y) - 1.f;

    float3 result = 0.f;
    switch (dispatchId.z)
    {
        case 0:
            result = float3(1.f, UV.y, -UV.x);
            break;
        case 1:
            result = float3(-1.f, UV.y, UV.x);
            break;
        case 2:
            result = float3(UV.x, 1.f, -UV.y);
            break;
        case 3:
            result = float3(UV.x, -1.f, UV.y);
            break;
        case 4:
            result = float3(UV.x, UV.y, 1.f);
            break;
        case 5:
            result = float3(-UV.x, UV.y, -1.f);
            break;
    }

    return normalize(result);
}

float CalculateRoughnessFromMipLevel(float mip, float mipCount)
{
    float levelFrom1x1 = mipCount - 1.f - mip;
    return exp2((1 - levelFrom1x1) / 1.2f);
}

[numthreads(32, 32, 1)]
void MainCS(uint3 dispatchId : SV_DispatchThreadID)
{
    float3 N = GetCubeMapTexCoord(dispatchId);
    float roughness = CalculateRoughnessFromMipLevel(MipIndex, MipCount);

    float3 color = 0.f;

    if (MipIndex == 0)
    {
        color = Input.SampleLevel(LinearSampler, N, 0.f);
    }
    else   
    {
        color = IntegrateCubeLDOnly(normalize(N), normalize(N), roughness * roughness);
    }

    RWOutput[dispatchId] = color;
}