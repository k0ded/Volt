#include "ViewData.hlsli"
#include "DepthPrePassCommon.hlsli"

float2 MainPS(in DepthPrePassPixelShaderInput input) : SV_Target0
{
    float3 currentPosNDC = input.currPosition.xyz / input.currPosition.w;
    float3 previousPosNDC = input.prevPosition.xyz / input.prevPosition.w;

    const float2 velocity = ((previousPosNDC.xy - View.prevFrameJitter) - (currentPosNDC.xy - View.currentFrameJitter)) * 0.5f;
    return velocity;
}