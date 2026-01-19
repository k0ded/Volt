#include "Material/MaterialShader.hlsli"

#include "RenderPipelineLegacy/GBufferCommon.hlsli"
#include "Utility/Utility.hlsli"

struct VSToPS
{
    float4 position : SV_Position;
    float4 currPosition : CURR_POSITION;
    float4 prevPosition : PREV_POSITION;
    float2 texCoords : TEXCOORD;
};

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

    MaterialEvaluationData evaluationData;
    evaluationData.texCoords = input.texCoords;

    EvaluatedMaterial evaluatedMaterial = EvaluateMaterial(evaluationData);
    EvaluateAlphaMask(evaluatedMaterial.albedo.a);

    return result;
}