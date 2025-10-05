#include "Vertex.hlsli"
#include "ViewData.hlsli"

#include "RenderScene/GPUScene.hlsli"

struct VSToPS
{
    float4 position : SV_Position;
    float4 currPosition : CURR_POSITION;
    float4 prevPosition : PREV_POSITION;
};

VSToPS MainVS(in Vertex input)
{
    const PrimitiveDrawData primitiveData = PrimitiveDrawDataBuffer[input.primitiveIndex];
    const PrimitiveDrawData prevPrimitiveData = PrevPrimitiveDrawDataBuffer[input.primitiveIndex];

    VSToPS result;
    result.position = mul(View.viewProjection, float4(primitiveData.transform.GetWorldPosition(input.position), 1.f));
    result.prevPosition = mul(View.prevViewProjection, float4(prevPrimitiveData.transform.GetWorldPosition(input.position), 1.f));
    result.currPosition = result.position;

    return result;
}

struct PSOutput
{
    [[vt::rg16f]] float2 velocity : SV_Target0;
    [[vt::d32f]];
};

PSOutput MainPS(in VSToPS input)
{
    float3 currentPosNDC = input.currPosition.xyz / input.currPosition.w;
    float3 previousPosNDC = input.prevPosition.xyz / input.prevPosition.w;

    PSOutput result;
    result.velocity = ((previousPosNDC.xy - View.prevFrameJitter) - (currentPosNDC.xy - View.currentFrameJitter)) * 0.5f;

    return result;
}