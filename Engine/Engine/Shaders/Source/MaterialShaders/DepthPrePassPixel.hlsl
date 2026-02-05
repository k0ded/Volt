#include "Material/MaterialShader.hlsli"

#include "Utility/Utility.hlsli"

struct VSToPS
{
    float4 position : SV_Position;
    float4 currPosition : CURR_POSITION;
    float4 prevPosition : PREV_POSITION;
    float2 texCoords : TEXCOORD;
};

float2 MainPS(in VSToPS input) : SV_Target0
{
    float3 currentPosNDC = input.currPosition.xyz / input.currPosition.w;
    float3 previousPosNDC = input.prevPosition.xyz / input.prevPosition.w;

    const float2 velocity = ((previousPosNDC.xy - View.prevFrameJitter) - (currentPosNDC.xy - View.currentFrameJitter)) * 0.5f;

    MaterialEvaluationData evaluationData;
    evaluationData.texCoords = input.texCoords;

    EvaluatedMaterial evaluatedMaterial = EvaluateMaterial(evaluationData);
    EvaluateAlphaMask(evaluatedMaterial.albedo.a);

    return velocity;
}