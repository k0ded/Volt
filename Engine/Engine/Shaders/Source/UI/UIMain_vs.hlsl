#include "Resources.hlsli"
#include "UIVertex.hlsli"

float4x4 ViewProjection;
vt::TextureSampler LinearSampler;

struct Output
{
    float4 position : SV_Position;
    float2 texCoords : TEXCOORD;
    float4 color : COLOR;
    uint imageHandle : IMAGEHANDLE;
};

Output main(in UIVertex input)
{
    Output output;
    output.position = mul(ViewProjection, input.position);
    output.color = input.color;
    output.texCoords = input.texCoords;
    output.imageHandle = input.imageHandle;

    return output;
}