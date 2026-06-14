#include "StaticSamplerStates.hlsli"

float2 invFullSize;
Texture2D<float> SrcTexture;
RWTexture2DArray<float> DstTexture;
SamplerState PointClampSampler;

[shader("compute")]
[numthreads(4, 4, 1)]
void MainCS(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    uint x, y, z;
    DstTexture.GetDimensions(x, y, z);

    if (dispatchThreadID.x >= x || dispatchThreadID.y >= y)
        return;

    float2 pos = floor(float2(dispatchThreadID.xy)) * 4.0 + 0.5;
    float2 uv = pos * invFullSize;
    
    float4 S0 = SrcTexture.GatherRed(PointClampSampler, uv);
    float4 S1 = SrcTexture.GatherRed(PointClampSampler, uv, int2(2, 0));
    float4 S2 = SrcTexture.GatherRed(PointClampSampler, uv, int2(0, 2));
    float4 S3 = SrcTexture.GatherRed(PointClampSampler, uv, int2(2, 2));

    DstTexture[uint3(dispatchThreadID, 0)] = S0.w;
    DstTexture[uint3(dispatchThreadID, 1)] = S0.z;
    DstTexture[uint3(dispatchThreadID, 2)] = S1.w;
    DstTexture[uint3(dispatchThreadID, 3)] = S1.z;
    DstTexture[uint3(dispatchThreadID, 4)] = S0.x;
    DstTexture[uint3(dispatchThreadID, 5)] = S0.y;
    DstTexture[uint3(dispatchThreadID, 6)] = S1.x;
    DstTexture[uint3(dispatchThreadID, 7)] = S1.y;
    DstTexture[uint3(dispatchThreadID, 8)] =  S2.w;
    DstTexture[uint3(dispatchThreadID, 9)] =  S2.z;
    DstTexture[uint3(dispatchThreadID, 10)] = S3.w;
    DstTexture[uint3(dispatchThreadID, 11)] = S3.z;
    DstTexture[uint3(dispatchThreadID, 12)] = S2.x;
    DstTexture[uint3(dispatchThreadID, 13)] = S2.y;
    DstTexture[uint3(dispatchThreadID, 14)] = S3.x;
    DstTexture[uint3(dispatchThreadID, 15)] = S3.y;
}