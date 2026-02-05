#include "Utility/Utility.hlsli"

struct ImGuiVertex
{
    float2 position : POSITION;
    float2 uv : TEXCOORD;
    [[vt::byte4]] uint4 color : COLOR;
};

struct VSToPS
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

float2 Scale;
float2 Translate;

VSToPS MainVS(ImGuiVertex input)
{
    VSToPS result;
    result.color = float4(input.color) / 255.f;
    result.uv = input.uv;
    result.position = float4(input.position * Scale + Translate, 0.f, 1.f);

    return result;
}

Texture2D<float4> Tex;
SamplerState Sampler;

float4 MainPS(VSToPS input) : SV_Target0
{
    // Since ImGui is using sRGB colors internally, we need to convert
    // them to linear space here. This is because we apply gamma correction when
    // copying to the swapchain.
    input.color.rgb = SRGBToLinear(input.color.rgb);
    const float4 color = input.color * Tex.Sample(Sampler, input.uv); 

    return color;
}