RWTexture2DArray<float3> RWOutput;
TextureCube<float3> Input;
SamplerState LinearSampler;

static const float PI = 3.14159265359;

float2 GetSample(uint index, uint numSamples, uint2 random)
{
	float E1 = frac( (float)index / numSamples + float( random.x & 0xffff ) / (1<<16) );
	float E2 = float( reversebits(index) ^ random.y ) * 2.3283064365386963e-10;
	return float2( E1, E2 );
}

void ImportanceSampleCosDir_N(float2 u, float3 N, out float3 L, out float NdotL, out float pdf)
{
    float3 upVector = abs(N.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
    float3 tangentX = normalize(cross(upVector, N));
    float3 tangentY = cross(N, tangentX);

    float u1 = u.x;
    float u2 = u.y;
    
    float r = sqrt(u1);
    float phi = u2 * PI * 2.f;

    L = float3(r * cos(phi), r * sin(phi), sqrt(max(0.f, 1.f - u1)));
    L = normalize(tangentX * L.y + tangentY * L.x + N * L.z);

    NdotL = dot(L, N);
    pdf = NdotL / PI;
}

float3 IntegrateDiffuseCube(in float3 N)
{
    const uint sampleCount = 64 * 1024;

    float3 accBrdf = 0.f;

    for (uint i = 0; i < sampleCount; i++)
    {
        float2 eta = GetSample(i, sampleCount, 0);
        float3 L;
        float NdotL;
        float pdf;
        
        ImportanceSampleCosDir_N(eta, N, L, NdotL, pdf);
        if (NdotL > 0.f)
        {   
            accBrdf += Input.SampleLevel(LinearSampler, L, 0.f);
        }
    }

    return accBrdf * (1.f / (float)sampleCount);
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

[numthreads(32, 32, 1)]
void MainCS(uint3 dispatchId : SV_DispatchThreadID)
{
    float3 N = GetCubeMapTexCoord(dispatchId);
    RWOutput[dispatchId] = IntegrateDiffuseCube(N);
}