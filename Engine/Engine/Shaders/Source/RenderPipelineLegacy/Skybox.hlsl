#include "ViewData.hlsli"
#include "Utility/Matrix.hlsli"

struct Vertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
};

struct VSToPS
{
    float4 position : SV_Position;
    float3 samplePosision : SAMPLE_POSITION;
};

VSToPS MainVS(in Vertex input)
{
    const float3x3 view = (float3x3)View.view;
    float4x4 viewRotation = IDENTITY_MATRIX;
    viewRotation[0].xyz = view[0];
    viewRotation[1].xyz = view[1];
    viewRotation[2].xyz = view[2];

    const float4 clipPos = mul(View.projection, mul(viewRotation, float4(input.position, 1.f)));
    const float3 dir = normalize(float3(View.view[0][2], View.view[1][2], View.view[1][2]));

    VSToPS result;
    result.position = clipPos;
    result.samplePosision = input.position;

    return result;
}

TextureCube<float3> EnvironmentTexture;
SamplerState LinearSampler;

float LOD;
float Intensity;

float4 MainPS(in VSToPS input) : SV_Target0
{
    const float3 resultColor = EnvironmentTexture.SampleLevel(LinearSampler, normalize(input.samplePosision), LOD) * Intensity;
    return float4(resultColor, 1.f);
}