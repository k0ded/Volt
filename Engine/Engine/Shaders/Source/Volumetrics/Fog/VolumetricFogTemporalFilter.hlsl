#include "Resources.hlsli"

#include "VolumetricFogCommon.hlsli"

struct Constants
{
    vt::RWTex3D<float4> rwLightScattering;
    vt::Tex3D<float4> prevLightScattering;
    vt::UniformBuffer<ViewData> viewData;
    vt::UniformBuffer<VolumetricFogParams> volumetricFogParams;

    float alpha;

    vt::TextureSampler pointSampler;
};

[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();
    const ViewData viewData = constants.viewData.Load();
    const VolumetricFogParams fogParams = constants.volumetricFogParams.Load();

    float4 scatteringExtinction = constants.rwLightScattering.Load(dispatchThreadID);

    float3 worldPositionNoJitter = GetWorldPositionFromFroxelCoordNoJitter(dispatchThreadID, fogParams, viewData);
    float4 screenSpaceCenterPrevious = mul(viewData.prevViewProjection, float4(worldPositionNoJitter, 1.f));
    float3 ndc = screenSpaceCenterPrevious.xyz / screenSpaceCenterPrevious.w;

    const float linearDepth = LinearizeDepth(ndc.z, viewData);
    const float depthUV = LinearDepthToExponentialUVDepth(fogParams.froxelNearPlane, fogParams.froxelFarPlane, linearDepth, fogParams.froxelVolumeDimensions.z);
    const float3 historyUV = float3(ndc.x * 0.5f + 0.5f, ndc.y * -0.5f + 0.5f, depthUV);
    
    if (all(historyUV >= 0.f) && all(historyUV <= 1.f))
    {
        float4 history = constants.prevLightScattering.SampleLevel(constants.pointSampler, historyUV, 0.f);
        history = max(history, scatteringExtinction);

        scatteringExtinction.rgb = lerp(history.rgb, scatteringExtinction.rgb, constants.alpha);
        scatteringExtinction.a = lerp(history.a, scatteringExtinction.a, constants.alpha);
    }

    constants.rwLightScattering.Store(dispatchThreadID, scatteringExtinction);
}