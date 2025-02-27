#include "Resources.hlsli"
#include "VolumetricFogCommon.hlsli"
#include "PBR/LightEvaluation.hlsli"
#include "MathConstants.hlsli"

struct Constants
{
    vt::RWTex3D<float4> rwLightScattering;

    vt::Tex3D<float4> scatteringExtinction;
    vt::UniformBuffer<ViewData> viewData;
    vt::UniformBuffer<VolumetricFogParams> volumetricFogParams;
    vt::UniformBuffer<DirectionalLightShadowData> directionalLightShadowData;
    vt::TypedBuffer<LightDrawData> lights;
    vt::TypedBuffer<int> visibleLights;
    vt::TextureSampler pointSampler;

    float phaseAnisotropy;

    BlueNoiseData blueNoiseData;
};

float PhaseFunctionHenyeyGreenstein(float g, float costh)
{
    const float n = 1.f - g * g;
    const float d = 4.f * PI * pow(1.f + g * g - 2.f * g * costh, 1.5f);
    return n / d;
}

[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();
 
    const ViewData viewData = constants.viewData.Load();
    const VolumetricFogParams fogParams = constants.volumetricFogParams.Load();

    const float3 worldPosition = GetWorldPositionFromFroxelCoord(dispatchThreadID, fogParams, constants.blueNoiseData, viewData);
    const float3 rcpFroxelDim = 1.f / float3(fogParams.froxelVolumeDimensions);

    const float3 uvw = float3(dispatchThreadID) * rcpFroxelDim;
    float4 scatteringExtinction = constants.scatteringExtinction.SampleLevel(constants.pointSampler, uvw, 0.f);

    const uint2 tileId = (float2(uvw.x, 1.f - uvw.y) * float2(viewData.renderSize)) / LIGHT_CULLING_TILE_SIZE;

    float3 evaluatedLighting = 0.f;

    if (scatteringExtinction.w > 0.01f)
    {
        const float3 V = normalize(viewData.cameraPosition.xyz - worldPosition);
        for (uint i = 0; i < viewData.lightCount; i++)
        {
            int lightIndex = GetLightBufferIndex(constants.visibleLights, viewData.tileCountX, i, tileId);
            if (lightIndex == -1)
            {
                break;
            }

            LightDrawData light = constants.lights.Load(lightIndex);
            
            const float3 unnormalizedL = light.position - worldPosition;
            const float3 L = normalize(unnormalizedL);
            const float VdotL = dot(V, -L);

            if (light.lightType == SceneLightType::SLT_Point)
            {
                if (length(unnormalizedL) < light.lightSpecific.x)
                {
                    const float invSqrRadius = 1.f / (light.lightSpecific.x * light.lightSpecific.x);
                    const float attenuation = GetDistanceAttenuation(unnormalizedL, invSqrRadius);
            
                    evaluatedLighting += light.color * light.intensity * attenuation * PhaseFunctionHenyeyGreenstein(constants.phaseAnisotropy, VdotL);
                } 
            }
            else if (light.lightType == SceneLightType::SLT_Directional)
            {
                evaluatedLighting += light.color * light.intensity * PhaseFunctionHenyeyGreenstein(constants.phaseAnisotropy, VdotL);
            }    
        }
    }

    const float3 scattering = scatteringExtinction.rgb * evaluatedLighting;
    constants.rwLightScattering.Store(dispatchThreadID, float4(scattering, scatteringExtinction.w));
}