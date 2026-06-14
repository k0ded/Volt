#include "ViewData.hlsli"
#include "Utility/FullscreenTriangleVertex.hlsli"
#include "Utility/Utility.hlsli"

Texture2D<float> NDCDepth;
SamplerState PointClampSampler;

struct InterleavedData
{
    float Depth : SV_Target0;
    float3 ViewSpaceNormal : SV_Target1;
};

float3 ReconstructViewPositionUV(float2 uv)
{
    float d = NDCDepth.SampleLevel(PointClampSampler, uv, 0);
    float4 clipSpace = float4(uv * 2 - 1, d, 1);
    float4 viewSpace = mul(View.inverseProjection, clipSpace);
    return viewSpace.xyz / viewSpace.w;
}

float3 MinDiff(float3 P, float3 Pr, float3 Pl)
{
    float3 V1 = Pr - P;
    float3 V2 = P - Pl;
    return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

[shader("pixel")]
InterleavedData MainPS(in FullscreenTriangleVertex input)
{
    const float3 P0 = ReconstructViewPositionUV(input.uv + float2(0, 0));
    const float3 Pr = ReconstructViewPositionUV(input.uv + float2(View.invRenderSize.x, 0));
    const float3 Pl = ReconstructViewPositionUV(input.uv + float2(-View.invRenderSize.x, 0));
    const float3 Pt = ReconstructViewPositionUV(input.uv + float2(0, View.invRenderSize.y));
    const float3 Pb = ReconstructViewPositionUV(input.uv + float2(0, -View.invRenderSize.y));

    InterleavedData OUT;
    OUT.Depth = P0.z;
    OUT.ViewSpaceNormal = normalize(cross(MinDiff(P0, Pr, Pl), MinDiff(P0, Pt, Pb))) * -0.5 + 0.5;
    return OUT;
}