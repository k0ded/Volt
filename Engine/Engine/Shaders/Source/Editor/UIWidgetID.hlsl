#include "Resources.hlsli"
#include "../UI/UIVertex.hlsli"

float4x4 ViewProjection;

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoords : TEXCOORD;
    uint widgetId : WIDGETID;
};

VSOutput MainVS(in UIVertex input)
{
    VSOutput output;
    output.position = mul(ViewProjection, input.position);
    output.texCoords = input.texCoords;
    output.widgetId = input.widgetId;

    return output;
}

uint MainPS(VSOutput input) : SV_Target0
{
    return input.widgetId;
}