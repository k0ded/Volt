#include "Vertex.hlsli"
#include "ViewData.hlsli"

#include "RenderScene/GPUScene.hlsli"
#include "Utility/ShadowMapping.hlsli"

struct VSToPS
{
    float4 position : SV_Position;
    uint target : SV_RenderTargetArrayIndex;
};

uint CascadeIndex;

VSToPS MainVS(in Vertex input)
{
    const PrimitiveDrawData primitiveData = PrimitiveDrawDataBuffer[input.primitiveIndex];

    VSToPS result;
    result.position = mul(CascadedDirectionalLightShadowMapping.viewProjections[CascadeIndex], float4(primitiveData.transform.GetWorldPosition(input.position), 1.f));
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