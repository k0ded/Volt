#include "Resources.hlsli"
#include "CommonBuffers.hlsli"

float4x4 ViewProjection;
vt::TextureSampler LinearSampler;

struct Input
{
    float4 position : SV_Position;
    float2 texCoords : TEXCOORD;
    float4 color : COLOR;
    uint imageHandle : IMAGEHANDLE;
};

float4 main(Input input) : SV_Target0
{
    vt::Tex2D<float4> texture = (vt::Tex2D<float4>)input.imageHandle;

    float4 color = texture.Sample(LinearSampler, input.texCoords);
    color.rgb *= input.color.rgb;
    color.a = input.color.a;

    return color;
}