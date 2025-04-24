#include "Vertex.hlsli"
#include "Resources.hlsli"

float4x4 InverseViewProjection;
float4x4 PreviousViewProjection;        
float2 RenderSize;
float2 InvRenderSize;
float2 JitterOffset;

vt::Tex2D<float> DepthTexture;
vt::TextureSampler PointSampler;

struct Output
{
    [[vt::rg16f]] float2 output : SV_Target0;
};

Output main(FullscreenTriangleVertex input)
{
    const float pixelDepth = DepthTexture.Sample(PointSampler, input.uv);
    
    if (pixelDepth < 0.00001f)
    {
        Output output;
        output.output = 0.f;
        
        return output;
    }

    const float x = input.uv.x * 2.f - 1.f;
    const float y = input.uv.y * 2.f - 1.f;

    const float4 projectedPos = float4(x, y, pixelDepth, 1.f);
    float4 worldPos = mul(InverseViewProjection, projectedPos);
    worldPos.xyz /= worldPos.w;

    const float4 reprojectedPos = mul(PreviousViewProjection, float4(worldPos.xyz, 1.f));
    const float2 reprojectedNDCPos = reprojectedPos.xy / reprojectedPos.w;
    const float2 reprojectedUV = reprojectedNDCPos * 0.5f + 0.5f;

    Output output;
    output.output = input.uv - reprojectedUV;

    return output;
}