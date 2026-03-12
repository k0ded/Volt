#include "Resources.hlsli"

#include "VolumetricFogCommon.hlsli"

vt::RWTex3D<float4> RWScatteringExtinction;
vt::UniformBuffer<ViewData> View;
vt::UniformBuffer<VolumetricFogParams> VolumetricFogParamsData;

float HeightFogDensity;
float HeightFogFalloff;
float3 HeightFogColor;

float4 EvaluateScatteringAndExtinctionFromColorDensity(float3 color, float density, float scatteringFactor)
{
    const float extinction = scatteringFactor * density;
    return float4(color * extinction, extinction);
}

[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const VolumetricFogParams volumetricFogParams = VolumetricFogParamsData.Load();
    const ViewData viewData = View.Load();

    float3 worldPosition = GetWorldPositionFromFroxelCoord(dispatchThreadID, volumetricFogParams, viewData);

    float4 scatteringExtinction = 0.f;

    float heightFog = HeightFogDensity * exp(-HeightFogFalloff * max(worldPosition.y, 0.f));
    scatteringExtinction += EvaluateScatteringAndExtinctionFromColorDensity(HeightFogColor, heightFog, volumetricFogParams.scatteringFactor);

    RWScatteringExtinction.Store(dispatchThreadID, scatteringExtinction);
}