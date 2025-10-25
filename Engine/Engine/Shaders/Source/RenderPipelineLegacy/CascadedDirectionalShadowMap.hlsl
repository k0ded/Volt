#include "Vertex.hlsli"
#include "ViewData.hlsli"
#include "Animation.hlsli"

#include "RenderScene/GPUScene.hlsli"
#include "Utility/ShadowMapping.hlsli"

struct ShadowVertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
    
    [[vt::inputIndex(2)]] uint4 influences : INFLUENCES;
    [[vt::inputIndex(2)]] float4 weights : WEIGHTS;

    [[vt::instance]] uint primitiveIndex : PRIMITIVEINDEX;
    uint instanceId : SV_InstanceID;
};

struct VSToPS
{
    float4 position : SV_Position;
    uint target : SV_RenderTargetArrayIndex;
};

uint CascadeIndex;

VSToPS MainVS(in ShadowVertex input)
{
    const PrimitiveDrawData primitiveData = PrimitiveDrawDataBuffer[input.primitiveIndex];

    float4x4 skinningMatrix = IDENTITY_MATRIX;
    if (primitiveData.isAnimated)
    {
        skinningMatrix = GetSkinningMatrix(primitiveData.boneOffset, input.influences, input.weights);
    }

    const float3 skinnedPosition = mul(skinningMatrix, float4(input.position, 1.f)).xyz;

    VSToPS result;
    result.position = mul(CascadedDirectionalLightShadowMapping.viewProjections[CascadeIndex], float4(primitiveData.transform.GetWorldPosition(skinnedPosition), 1.f));
    result.target = CascadeIndex;

    return result;
}

struct ColorOutput
{
    [[vt::d32f]];
};

ColorOutput MainPS(VSToPS input)
{
    ColorOutput output;
    return output;
}