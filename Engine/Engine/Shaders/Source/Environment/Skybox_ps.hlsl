#include "Resources.hlsli"
#include "Structures.hlsli"
#include "Vertex.hlsli"
#include "Exposure/Exposure.hlsli"

#include "Volumetrics/Fog/VolumetricFogCommon.hlsli"

struct Constants
{
    vt::TypedBuffer<VertexPositionData> vertexPositions;
    vt::UniformBuffer<ViewData> viewData;

    vt::TexCube<float3> environmentTexture;
    vt::TextureSampler linearSampler;

    float lod;
    float intensity;

    vt::Tex2D<float> sceneDepth;

    // Volumetric Fog
    vt::UniformBuffer<VolumetricFogParams> volumetricFogParams;
    vt::Tex3D<float4> integratedFogVolume;
    vt::TextureSampler pointSampler;
};

struct Input
{
    float4 position : SV_Position;
    float3 samplePosition : SAMPLE_POSITION;
};

struct Output
{
    [[vt::r11f_g11f_b10f]] float3 output : SV_Target0;
};

Output main(Input input)
{
    const Constants constants = GetConstants<Constants>();
    const ViewData viewData = constants.viewData.Load();


    float3 result = constants.environmentTexture.SampleLevel(constants.linearSampler, input.samplePosition * 0.01f, constants.lod) * constants.intensity;

    // Volumetric Fog
    const uint2 screenCoords = input.position.xy;
    const float2 screenUV = float2(screenCoords.x * viewData.invRenderSize.x, 1.f - (screenCoords.y * viewData.invRenderSize.y));
    const float sceneDepth = constants.sceneDepth.Load(int3(screenCoords, 0));
 
    const VolumetricFogParams fogParams = constants.volumetricFogParams.Load();

    //result = ApplyVolumetricFog(screenUV, 0.00001f, result, viewData, fogParams, constants.pointSampler, constants.integratedFogVolume);

    Output output;
    output.output = result;
    return output;
}