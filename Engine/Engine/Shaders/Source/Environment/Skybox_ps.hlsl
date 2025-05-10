#include "Resources.hlsli"
#include "Structures.hlsli"
#include "Vertex.hlsli"
#include "Exposure/Exposure.hlsli"

#include "Volumetrics/Fog/VolumetricFogCommon.hlsli"

vt::TypedBuffer<VertexPositionData> VertexPositions;
vt::UniformBuffer<ViewData> View;

vt::TexCube<float3> EnvironmentTexture;
vt::TextureSampler LinearSampler;

float LOD;
float Intensity;

vt::Tex2D<float> SceneDepth;

// Volumetric Fog
vt::UniformBuffer<VolumetricFogParams> VolumetricFogParamsData;
vt::Tex3D<float4> IntegratedFogVolume;
vt::TextureSampler PointSampler;

struct Input
{
    float4 position : SV_Position;
    float3 samplePosition : SAMPLE_POSITION;
};

struct Output
{
    [[vt::rgba16f]] float4 output : SV_Target0;
};

Output main(Input input)
{
    const ViewData viewData = View.Load();

    float3 result = EnvironmentTexture.SampleLevel(LinearSampler, input.samplePosition * 0.01f, LOD) * Intensity;

    // Volumetric Fog
    const uint2 screenCoords = input.position.xy;
    const float2 screenUV = float2(screenCoords.x * viewData.invRenderSize.x, 1.f - (screenCoords.y * viewData.invRenderSize.y));
    const float sceneDepth = SceneDepth.Load(int3(screenCoords, 0));
 
    const VolumetricFogParams fogParams = VolumetricFogParamsData.Load();

    //result = ApplyVolumetricFog(screenUV, 0.00001f, result, viewData, fogParams, constants.pointSampler, constants.integratedFogVolume);

    Output output;
    output.output = float4(result, 1.f);
    return output;
}