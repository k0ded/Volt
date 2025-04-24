#include "Resources.hlsli"
#include "CommonBuffers.hlsli"

float4x4 ViewProjection;
vt::TextureSampler LinearSampler;

struct Output
{
    [[vt::rgba8]] float4 color : SV_Target;
    [[vt::d32f]];
};

struct Input
{
    float4 position : SV_Position;
    float2 texCoords : TEXCOORD;
    float4 color : COLOR;
    uint imageHandle : IMAGEHANDLE;
};

Output main(Input input)
{
    vt::Tex2D<float4> texture = (vt::Tex2D<float4>)input.imageHandle;

    Output output;
    output.color = texture.Sample(LinearSampler, input.texCoords);
    output.color.rgb *= input.color.rgb;
    output.color.a = input.color.a;

    return output;
}