#include "ViewData.hlsli"
#include "DepthPrePassCommon.hlsli"

struct PSOutput
{
    [[vt::rg16f]] float2 velocity : SV_Target0;
    [[vt::d32f]];
};

PSOutput MainPS(in DepthPrePassPixelShaderInput input)
{
    float3 currentPosNDC = input.currPosition.xyz / input.currPosition.w;
    float3 previousPosNDC = input.prevPosition.xyz / input.prevPosition.w;

    PSOutput result;
    result.velocity = ((previousPosNDC.xy - View.prevFrameJitter) - (currentPosNDC.xy - View.currentFrameJitter)) * 0.5f;

    return result;
}