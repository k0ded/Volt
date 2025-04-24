#include "Resources.hlsli"

#include "VolumetricFogCommon.hlsli"

vt::RWTex3D<float4> RWLightScattering;
vt::Tex3D<float4> PrevLightScattering;
vt::UniformBuffer<ViewData> View;
vt::UniformBuffer<VolumetricFogParams> VolumetricFogParamsData;

float Alpha;

vt::TextureSampler PointSampler;

[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const ViewData viewData = View.Load();
    const VolumetricFogParams fogParams = VolumetricFogParamsData.Load();

    float4 scatteringExtinction = RWLightScattering.Load(dispatchThreadID);

    float3 worldPositionNoJitter = GetWorldPositionFromFroxelCoordNoJitter(dispatchThreadID, fogParams, viewData);
    float4 screenSpaceCenterPrevious = mul(viewData.prevViewProjection, float4(worldPositionNoJitter, 1.f));
    float3 ndc = screenSpaceCenterPrevious.xyz / screenSpaceCenterPrevious.w;

    const float linearDepth = LinearizeDepth(ndc.z, viewData);
    const float depthUV = LinearDepthToExponentialUVDepth(fogParams.froxelNearPlane, fogParams.froxelFarPlane, linearDepth, fogParams.froxelVolumeDimensions.z);
    const float3 historyUV = float3(ndc.x * 0.5f + 0.5f, ndc.y * -0.5f + 0.5f, depthUV);
    
    if (all(historyUV >= 0.f) && all(historyUV <= 1.f))
    {
        float4 history = PrevLightScattering.SampleLevel(PointSampler, historyUV, 0.f);
        history = max(history, scatteringExtinction);

        scatteringExtinction.rgb = lerp(history.rgb, scatteringExtinction.rgb, Alpha);
        scatteringExtinction.a = lerp(history.a, scatteringExtinction.a, Alpha);
    }

    RWLightScattering.Store(dispatchThreadID, scatteringExtinction);
}