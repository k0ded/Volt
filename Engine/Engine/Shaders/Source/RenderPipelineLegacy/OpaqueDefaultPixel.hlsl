#include "RenderPipelineLegacy/GBufferCommon.hlsli"

GBufferPixelShaderOutput MainPS(in GBufferPixelShaderInput input)
{
    GBufferPixelShaderOutput result;
    result.albedo = float4(0.8f.xxx, 1.f);
    result.normal = float4(input.normal * 0.5f + 0.5f, 1.f);
    result.material = float2(0.8f, 0.f);

    return result;
}