#pragma once

struct DepthPrePassPixelShaderInput
{
    float4 position : SV_Position;
    float4 currPosition : CURR_POSITION;
    float4 prevPosition : PREV_POSITION;
    float2 texCoords : TEXCOORD;
};

