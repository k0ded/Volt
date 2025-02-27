#include "Resources.hlsli"

#include "VolumetricFogCommon.hlsli"

struct Constants
{
    vt::RWTex3D<float4> rwScatteringExtinction;
    vt::UniformBuffer<ViewData> viewData;
    vt::UniformBuffer<VolumetricFogParams> volumetricFogParams;

    float heightFogDensity;
    float heightFogFalloff;
    float3 heightFogColor;

    BlueNoiseData blueNoiseData;
};

float4 EvaluateScatteringAndExtinctionFromColorDensity(float3 color, float density, float scatteringFactor)
{
    const float extinction = scatteringFactor * density;
    return float4(color * extinction, extinction);
}

[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();

    const VolumetricFogParams volumetricFogParams = constants.volumetricFogParams.Load();
    const ViewData viewData = constants.viewData.Load();

    float3 worldPosition = GetWorldPositionFromFroxelCoord(dispatchThreadID, volumetricFogParams, constants.blueNoiseData, viewData);

    float4 scatteringExtinction = 0.f;

    float heightFog = constants.heightFogDensity * exp(-constants.heightFogFalloff * max(worldPosition.y, 0.f));
    scatteringExtinction += EvaluateScatteringAndExtinctionFromColorDensity(constants.heightFogColor, heightFog, volumetricFogParams.scatteringFactor);

    constants.rwScatteringExtinction.Store(dispatchThreadID, scatteringExtinction);
}